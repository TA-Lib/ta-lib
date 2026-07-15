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
      children: ["functions/", "api/", "api/stream/", "faq/"],
    },
    "about/",
  ],
});
