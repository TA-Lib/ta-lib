import { defineUserConfig } from "vuepress";

import theme from "./theme.js";
import { viteBundler } from "@vuepress/bundler-vite";

export default defineUserConfig({
  base: "/",

  head: [
    ["link", { rel: "icon", type: "image/png", sizes: "32x32", href: "/favicon-32x32.png" }],
    ["link", { rel: "icon", type: "image/png", sizes: "16x16", href: "/favicon-16x16.png" }],
    ["link", { rel: "icon", href: "/favicon.ico" }],
    ["link", { rel: "apple-touch-icon", href: "/apple-touch-icon.png" }],
  ],

  // ta_codegen writes the generated function pages directly into src/functions
  // (real files, in-tree), so no symlink and no Vite vue-alias workaround are needed.
  bundler: viteBundler(),

  locales: {
    "/": {
      lang: "en-US",
      title: "TA-Lib.org",
      description:
        "Open-Source library for technical analysis of time series and trading data",
    },
  },

  theme,
});
