import { ComponentChildren } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { HashRouter } from './lib/router';
import { Header } from './components/Header';
import { Dashboard } from './pages/Dashboard';
import { Settings } from './pages/Settings';
import { Status } from './pages/Status';

export function App() {
  const [page, setPage] = useState<ComponentChildren>(<Dashboard />);

  useEffect(() => {
    const router = new HashRouter({
      '/': () => <Dashboard />,
      '/settings': () => <Settings />,
      '/status': () => <Status />,
    }, setPage);
    return () => router.destroy();
  }, []);

  return (
    <div class="app">
      <Header currentPage={location.hash.slice(1) || '/'} onNavigate={setPage} />
      <main class="main-content">{page}</main>
    </div>
  );
}
