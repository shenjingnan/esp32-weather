const BASE_URL = '';

class ApiClient {
  async get<T>(path: string): Promise<T> {
    const res = await fetch(`${BASE_URL}${path}`);
    if (!res.ok) throw new Error(`HTTP ${res.status}: ${path}`);
    return res.json();
  }

  async post(path: string, data?: unknown): Promise<unknown> {
    const res = await fetch(`${BASE_URL}${path}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data),
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}: ${path}`);
    return res.json();
  }
}

export const api = new ApiClient();
