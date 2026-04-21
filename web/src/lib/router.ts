import { ComponentChildren } from 'preact';

type RouteRenderer = () => ComponentChildren;

export class HashRouter {
  private routes: Record<string, RouteRenderer>;
  private setPage: (page: ComponentChildren) => void;
  private handler: (() => void) | null = null;

  constructor(routes: Record<string, RouteRenderer>, setPage: (page: ComponentChildren) => void) {
    this.routes = routes;
    this.setPage = setPage;

    this.handler = () => this.handleRoute();
    window.addEventListener('hashchange', this.handler);
    this.handleRoute();
  }

  private handleRoute() {
    const path = location.hash.slice(1) || '/';
    const renderer = this.routes[path];
    if (renderer) {
      this.setPage(renderer());
    }
  }

  destroy() {
    if (this.handler) {
      window.removeEventListener('hashchange', this.handler);
      this.handler = null;
    }
  }
}
