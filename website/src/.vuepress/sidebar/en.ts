import { sidebar } from "vuepress-theme-hope";

export const enSidebar = sidebar({
  "/": [
    { text: "Home", link: "" },
    "install/",
    {
      text: "Docs",
      link: "functions/",
      children: ["functions/", "api/", "wrappers/", "faq/"],
    },
    "about/",
  ],
});
