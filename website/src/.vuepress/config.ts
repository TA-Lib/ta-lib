import * as path from "path";
import { fileURLToPath } from "url";
import { defineUserConfig } from "vuepress";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
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

  bundler: viteBundler({
    viteOptions: {
      // src/functions is a symlink to ../../docs/functions (ta_codegen's
      // generated output — kept in its canonical location so the generator
      // needs no changes). Vite/Rolldown resolve bare imports from a
      // symlinked page's real (out-of-tree) path, where "vue" isn't reachable
      // via the normal node_modules walk-up, so alias it explicitly (same fix
      // suibase uses for its symlinked demo pages).
      resolve: {
        alias: [
          {
            find: /^vue$/,
            replacement: path.resolve(__dirname, "../../node_modules/vue"),
          },
          {
            find: /^vue\//,
            replacement:
              path.resolve(__dirname, "../../node_modules/vue") + "/",
          },
        ],
      },
    },
    vuePluginOptions: {},
  }),

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
