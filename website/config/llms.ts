import { readFileSync } from "node:fs";
import { readFile, writeFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import {
  generateTOCLink,
  llmsPlugin,
  remarkImportCode,
  remarkInclude,
  remarkPlease,
  type LLMPage,
  type LLMState,
} from "@vuepress/plugin-llms";
import matter from "gray-matter";
import { remark } from "remark";
import remarkMath from "remark-math";
import type { Page, Plugin } from "vuepress";

const DOMAIN = "https://ta-lib.org";

// Agent-only. The homepage is the human overview; these facts reach agents at the top of
// /llms.txt and /llms-full.txt and appear on no page.
const keyFacts = readFileSync(
  join(dirname(fileURLToPath(import.meta.url)), "llms-key-facts.md"),
  "utf8",
).trim();

// Agent-only files, copied verbatim from .vuepress/public/agents/ and linked from nowhere
// but /llms.txt.
const MIGRATION_GUIDES = [
  ["pandas-ta", "Move pandas-ta code to pandas-ta-classic or to TA-Lib: the name map, and what changes."],
  ["ta", "Move code from the ta package to TA-Lib: the name map, and a replacement for add_all_ta_features."],
  ["talipp", "Move a talipp incremental pipeline to TA-Lib's streaming handles: the concept and name maps."],
];

// The /functions/ pages that are not one function's page.
const FUNCTION_SECTION_PAGES = new Set(["/functions/", "/functions/stability.html"]);
const isFunctionPage = (page: Page): boolean =>
  page.path.startsWith("/functions/") && !FUNCTION_SECTION_PAGES.has(page.path);

// Within a section, in the sidebar's order.
const PAGE_ORDER = [
  "/spec/inputs-outputs/",
  "/spec/lookback/",
  "/spec/auto-stabilization/",
  "/spec/streaming/",
  "/spec/abstract/",
  "/spec/settings-threads/",
  "/spec/errors/",
  "/spec/versions/",
  "/api/",
  "/api/stream/",
  "/api/rust/",
  "/api/rust/stream/",
  "/api/java/",
  "/api/java/stream/",
  "/api/csharp/",
  "/api/csharp/stream/",
  "/api/abstract/",
  "/api/unstable-period/",
  "/functions/stability.html",
  "/api/candle-settings/",
];

const SECTIONS: [string, (page: Page) => boolean][] = [
  ["API and concepts", (page) => page.path.startsWith("/api/") || page.path === "/functions/stability.html"],
  ["Specification", (page) => page.path.startsWith("/spec/")],
  ["Functions", (page) => page.path.startsWith("/functions/")],
  ["Project", () => true],
];

const sectionOf = (page: Page): number => SECTIONS.findIndex(([, belongs]) => belongs(page));

// A section's index page first, then the listed pages, then the rest by path.
const orderOf = (page: Page): number =>
  page.path === "/spec/" || page.path === "/functions/"
    ? -1
    : PAGE_ORDER.indexOf(page.path) + 1 || PAGE_ORDER.length + 1;

const byRank = (a: Page, b: Page): number =>
  sectionOf(a) - sectionOf(b) || orderOf(a) - orderOf(b) || a.path.localeCompare(b.path);

const toc = (pages: LLMPage[], state: LLMState): string => {
  const sorted = [...pages].sort(byRank);
  const blocks = SECTIONS.map(([title], index) => {
    const lines = sorted
      .filter((page) => sectionOf(page) === index)
      .map((page) => generateTOCLink(page, state).trimEnd());
    return lines.length ? `### ${title}\n\n${lines.join("\n")}` : "";
  });
  const migration = MIGRATION_GUIDES.map(
    ([name, what]) => `- [Migrating from ${name}](${DOMAIN}/agents/migrate/${name}.md): ${what}`,
  );
  blocks.splice(3, 0, `### Migrating from other libraries\n\n${migration.join("\n")}`);
  return blocks.filter(Boolean).join("\n\n");
};

// /llms-full.txt without the function pages: their full text is far beyond what an agent
// reads at once, and /functions/index.md, which is included, lists and links every one.
// Built from the Markdown twins, so it runs after the llms plugin has written them. The
// filter must stay the plugin's own page selection: a narrower one drops a page silently.
const llmsFull: Plugin = {
  name: "ta-lib-llms-full",
  onGenerated: async (app) => {
    const pages = app.pages
      .filter(
        (page) =>
          page.pathLocale === "/" &&
          page.filePath?.endsWith(".md") &&
          page.frontmatter.llmstxt !== false &&
          matter(page.content).content.trim() !== "" &&
          !isFunctionPage(page),
      )
      .sort(byRank);
    const bodies = await Promise.all(
      pages.map((page) => readFile(app.dir.dest(page.htmlFilePathRelative.replace(/\.html$/, ".md")), "utf8")),
    );
    const { locales, ...site } = app.siteData;
    const { title, description } = { ...site, ...locales["/"] };
    const head = `# ${title}\n\n> ${description}\n\n## Key facts\n\n${keyFacts}\n`;
    await writeFile(app.dir.dest("llms-full.txt"), [head, ...bodies].join("\n---\n\n"));
  },
};

// The plugin's own remark pass has no math syntax, so it rewrites LaTeX on the function
// pages (`_{t-1}` becomes emphasis). This repeats that pass, every step of it, with
// remark-math added. It runs in its own hook because the plugin's transformMarkdown is
// synchronous and this pass is not.
const mathSafe = new Map<string, string>();

const llmsMath: Plugin = {
  name: "ta-lib-llms-math",
  onGenerated: async (app) => {
    await Promise.all(
      app.pages
        .filter((page) => page.filePath?.endsWith(".md"))
        .map(async (page) => {
          const file = await remark()
            .use(remarkMath)
            .use(remarkPlease("unwrap", "llm-only"))
            .use(remarkPlease("remove", "llm-exclude"))
            .use(
              remarkInclude(dirname(page.filePath!), {
                resolvePath: (file: string) => file,
                deep: false,
                resolveLinkPath: true,
                resolveImagePath: true,
                useComment: true,
              }),
            )
            .use(remarkImportCode(dirname(page.filePath!), {}))
            .process(matter(page.content).content);
          mathSafe.set(page.filePath!, String(file));
        }),
    );
  },
};

export default [
  llmsMath,
  llmsPlugin({
    domain: DOMAIN,

    // The default drops every raw HTML node with its text. The C API and unstable-period
    // pages lay out their prose and signatures in <p> and <pre>, so little more than their
    // headings would reach agents.
    stripHTML: false,

    llmsFullTxt: false,

    // The default template's empty {alternateLinks} fuses the description into the next line.
    llmsTxtTemplate:
      "# {title}\n\n{description}\n\n## Key facts\n\n{keyFacts}\n\n## Table of Contents\n\n{toc}\n",
    llmsTxtTemplateGetter: { keyFacts, toc },

    // Pages carry their title in frontmatter, so the twins get it as an H1.
    transformMarkdown: (_markdown, page) => {
      const md = mathSafe.get(page.filePath!);
      if (md === undefined) throw new Error(`llms: no math-safe Markdown for ${page.filePath}`);
      return /^# /.test(md.trimStart()) ? md : `# ${page.title}\n\n${md}`;
    },
  }),
  llmsFull,
];
