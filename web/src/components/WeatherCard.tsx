interface WeatherData {
  location: string;
  temperature: number;
  humidity: number;
  description: string;
  icon: string;
}

export function WeatherCard({ data }: { data: WeatherData }) {
  const iconMap: Record<string, string> = {
    sunny: '\u2600\ufe0f',
    cloudy: '\u2601\ufe0f',
    rainy: '\ud83c\udf27\ufe0f',
    snowy: '\u2744\ufe0f',
    foggy: '\ud83c\udf2b',
    windy: '\ud83d\udca8',
  };

  return (
    <div class="card weather-main">
      <div class="weather-icon">{iconMap[data.icon] || '\u2600\ufe0f'}</div>
      <div class="weather-temp">{data.temperature}&deg;C</div>
      <div class="weather-desc">{data.description}</div>
      <p style={{ color: 'var(--color-text-secondary)', fontSize: '14px', marginTop: '8px' }}>
        湿度 {data.humidity}%
      </p>
    </div>
  );
}
