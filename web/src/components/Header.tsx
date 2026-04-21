import { ComponentChildren } from 'preact';

interface HeaderProps {
  currentPage: string;
  onNavigate: (page: ComponentChildren) => void;
}

const NAV_ITEMS = [
  { path: '/', label: '首页' },
  { path: '/settings', label: '设置' },
  { path: '/status', label: '状态' },
];

export function Header({ currentPage, onNavigate: _onNavigate }: HeaderProps) {
  return (
    <header class="header">
      <h1>ESP32 Weather</h1>
      <nav class="header-nav">
        {NAV_ITEMS.map((item) => (
          <button
            key={item.path}
            class={`nav-item${currentPage === item.path ? ' active' : ''}`}
            onClick={() => {
              location.hash = item.path;
              // Navigation is handled by HashRouter in App.tsx
            }}
          >
            {item.label}
          </button>
        ))}
      </nav>
    </header>
  );
}
