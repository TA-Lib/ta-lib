import { defineComponent, h, onMounted, ref } from "vue";
import { defineClientConfig, withBase } from "vuepress/client";

// Recover a tab that was open across a deploy.
//
// Every route component is a content-hashed chunk (`/assets/rsi-DN8xgYjT.js`)
// and the gh-pages deploy replaces the whole tree, so an already-loaded tab
// holds filenames the server no longer has. The router awaits that import
// inside `beforeResolve`; the 404 rejects the navigation, and nprogress —
// which starts its bar in `beforeEach` and only clears it in `afterEach` —
// leaves the bar running across the top of a page that never changes. A
// reload is what clears it by hand; this does the same thing automatically.
const STALE_CHUNK =
  /dynamically imported module|Importing a module script failed|Unable to preload/iu;

// The target of the reload in flight. A chunk missing for some other reason
// must not turn one click into a reload loop, so each target is retried at
// most once; any successful navigation clears the mark.
const RELOAD_TARGET = "vp-reload-stale-chunk";

// A function page's chart. The SVG is fetched on mount and inlined, so its
// own stylesheet picks up the theme's `--vp-c-*` and the page's HTML and JS
// chunk carry none of it; until then the figure holds the chart's height so
// nothing below it moves. `<noscript>` holds an `<img>` of the same file for a
// reader without JS. `label` names the chart and `alt` is the drawing
// sentence; the page shows the sentence nowhere else, so a screen reader must
// get it from the inlined figure as well as from the `<img>`.
const TaChart = defineComponent({
  name: "TaChart",
  props: {
    src: { type: String, required: true },
    ratio: { type: String, default: "720:285" },
    label: { type: String, default: "" },
    alt: { type: String, default: "" },
  },
  setup(props) {
    const url = withBase(props.src);
    const svg = ref("");
    const failed = ref(false);

    onMounted(async () => {
      try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(response.statusText);
        const text = await response.text();
        if (!/^\s*(?:<\?xml[^>]*>\s*)?<svg[\s>]/u.test(text)) throw new Error("not an SVG");
        svg.value = text;
      } catch {
        failed.value = true;
      }
    });

    return () => {
      const [w, hgt] = props.ratio.split(":");
      // The figure carries the aspect ratio and the minimum width, and holds the
      // svg or the img at width 100%, so the space reserved before the fetch is
      // the space the chart takes after it, at every viewport width.
      const fig = { class: "ta-chart-fig", style: { aspectRatio: `${w} / ${hgt}` } };
      const box = (child: ReturnType<typeof h>) => h("div", { class: "ta-chart-box" }, child);
      if (svg.value) return box(h("div", { ...fig, role: "img", "aria-label": props.alt ? `${props.label}. ${props.alt}` : props.label, innerHTML: svg.value }));
      if (failed.value) return h("div", { class: "ta-chart-box" }, h("a", { href: url }, "Chart (SVG)"));
      const alt = props.alt.replace(/&/gu, "&amp;").replace(/"/gu, "&quot;");
      return box(h("div", fig, h("noscript", { innerHTML: `<img src="${url}" alt="${alt}">` })));
    };
  },
});

export default defineClientConfig({
  enhance({ app, router }) {
    // The llms plugin removes these tags only in some positions; anywhere else Vue meets
    // them as unknown components: <llm-only> text then shows to readers, and a block
    // <llm-exclude> vanishes from the server-rendered page. Both renders need these.
    app.component("llm-only", () => null);
    app.component("llm-exclude", (_props, { slots }) => slots.default?.());
    app.component("ta-chart", TaChart);

    if (!__VUEPRESS_SSR__) {
      // The (i) badge inside a fold's summary explains the fold; a tap on it must not
      // also toggle the fold.
      document.addEventListener("click", (event) => {
        const target = event.target as Element | null;
        if (target?.closest?.("summary .flag-tip")) event.preventDefault();
      });
    }

    if (__VUEPRESS_SSR__) return;

    router.onError((error: Error, to) => {
      if (!STALE_CHUNK.test(error.message)) return;
      if (sessionStorage.getItem(RELOAD_TARGET) === to.fullPath) return;

      sessionStorage.setItem(RELOAD_TARGET, to.fullPath);
      // Every route path is a real file in the deploy, so this lands on the
      // page that was clicked, served by the new build.
      location.replace(to.fullPath);
    });

    router.afterEach(() => {
      sessionStorage.removeItem(RELOAD_TARGET);
    });
  },
});
