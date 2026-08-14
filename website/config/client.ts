import { defineClientConfig } from "vuepress/client";

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

export default defineClientConfig({
  enhance({ router }) {
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
