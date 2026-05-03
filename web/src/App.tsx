import { ComponentChildren } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { HashRouter } from './lib/router';
import { Header } from './components/Header';
import { Dashboard } from './pages/Dashboard';
import { Settings } from './pages/Settings';
import { WifiConfig } from './pages/WifiConfig';
import { Status } from './pages/Status';

export function App() {
  const [page, setPage] = useState<ComponentChildren>(<Dashboard />);
  const [wifiReady, setWifiReady] = useState<boolean | null>(null);

  useEffect(() => {
    fetch('/api/wifi/status')
      .then((r) => r.json())
      .then((status) => setWifiReady(status.status === 'connected'))
      .catch(() => setWifiReady(false));
  }, []);

  useEffect(() => {
    if (!wifiReady) return;

    const router = new HashRouter({
      '/': () => <Dashboard />,
      '/wifi': () => <WifiConfig />,
      '/settings': () => <Settings />,
      '/status': () => <Status />,
    }, setPage);
    return () => router.destroy();
  }, [wifiReady]);

  if (wifiReady === null) {
    return <div class="loading">加载中...</div>;
  }

  if (!wifiReady) {
    return (
      <div class="wifi-portal">
        <WifiConfig onConnected={() => setWifiReady(true)} />
      </div>
    );
  }

  return (
    <div class="app">
      <Header currentPage={location.hash.slice(1) || '/'} onNavigate={setPage} />
      <main class="main-content">{page}</main>
    </div>
  );
}
