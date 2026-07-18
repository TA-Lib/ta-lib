import { sidebar } from "vuepress-theme-hope";

export const enSidebar = sidebar({
  "/": [
    { text: "Home", link: "" },
    {
      text: "Install",
      link: "install/",
      children: [{ text: "C/C++", link: "install/" }, "wrappers/"],
    },
    {
      text: "Docs",
      link: "functions/",
      children: [
        "functions/",
        {
          text: "C/C++ API",
          collapsible: true,
          children: ["api/", "api/stream/"],
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
    "about/",
  ],
});
