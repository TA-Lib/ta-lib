import { sidebar } from "vuepress-theme-hope";

export const enSidebar = sidebar({
  "/": [
    { text: "Home", link: "" },
    // Deliberately childless: /install/ is a router page listing every native
    // and every wrapper, so the sub-pages it links to would only duplicate it.
    { text: "Install", link: "install/" },
    {
      text: "Docs",
      link: "functions/",
      children: [
        "functions/",
        {
          text: "C/C++ API",
          collapsible: true,
          // The install page lives at /install/c/ (reached from the /install/
          // router too); it is listed here because that is where a reader
          // already in the C docs looks for it.
          children: ["install/c/", "api/", "api/stream/"],
        },
        {
          text: "Rust API",
          collapsible: true,
          children: ["api/rust/", "api/rust/stream/"],
        },
        {
          text: "Java API",
          collapsible: true,
          children: ["api/java/", "api/java/stream/"],
        },
        "faq/",
      ],
    },
    "contribute/",
    "about/",
  ],
});
