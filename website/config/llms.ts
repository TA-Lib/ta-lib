import { dirname } from "node:path";
import {
  llmsPlugin,
  remarkImportCode,
  remarkInclude,
  remarkPlease,
} from "@vuepress/plugin-llms";
import matter from "gray-matter";
import { remark } from "remark";
import remarkMath from "remark-math";

// Writes /llms.txt, /llms-full.txt and a Markdown twin of every page (/api/rust/index.md).
export default llmsPlugin({
  domain: "https://ta-lib.org",

  // The default drops every raw HTML node with its text. The C API and unstable-period
  // pages lay out their prose and signatures in <p> and <pre>, so little more than their
  // headings would reach agents.
  stripHTML: false,

  // The default template's empty {alternateLinks} fuses the description into the next line.
  llmsTxtTemplate: "# {title}\n\n{description}\n\n## Table of Contents\n\n{toc}",

  // The plugin's own remark pass has no math syntax, so it rewrites LaTeX on the function
  // pages (`_{t-1}` becomes emphasis). This repeats that pass, every step of it, with
  // remark-math added. Pages carry their title in frontmatter, so the twins get it as an H1.
  transformMarkdown: (_markdown, page) => {
    const md = String(
      remark()
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
        .processSync(matter(page.content).content),
    );
    return /^# /.test(md.trimStart()) ? md : `# ${page.title}\n\n${md}`;
  },
});
