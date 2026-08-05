import { hopeTheme } from "vuepress-theme-hope";
import { enNavbar } from "./navbar/index.js";
import { enSidebar } from "./sidebar/index.js";

export default hopeTheme({
  hostname: "https://ta-lib.org",

  author: {
    name: "TA-Lib",
    url: "https://ta-lib.org",
  },

  repo: "TA-Lib/ta-lib",
  repoLabel: "Github (Core C Library)",

  contributors: false,
  editLink: false,
  pageInfo: false,
  breadcrumb: false,

  docsDir: "website/src/",

  hotReload: true,

  markdown: {
    align: true,
    attrs: true,
    gfm: true,
    include: true,
    mark: true,
    mermaid: false,
    sub: true,
    sup: true,
    vPre: true,

    figure: true,
    imgLazyload: true,
    imgMark: true,
    imgSize: true,

    math: true,

    alert: true,
    hint: true,

    tabs: true,
    codeTabs: true,
  },

  locales: {
    "/": {
      navbar: enNavbar,
      sidebar: enSidebar,

      footer:
        '<a href="https://github.com/TA-Lib/ta-lib">TA-Lib on Github</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="https://discord.com/invite/Erb6SwsVbH">TA-Lib on Discord</a>',

      copyright: false,

      displayFooter: true,

      metaLocales: {
        editLink: "Edit this page on GitHub",
      },
    },
  },

  plugins: {
    git: false,

    // Self-referencing canonical on every page: the string is used as a prefix
    // and the page path is appended. Needed before the per-language API pages
    // land, so near-identical siblings each declare themselves as the original.
    seo: {
      canonical: "https://ta-lib.org",
    },

    icon: {
      assets: "iconify",
    },

    slimsearch: true,
  },
});
