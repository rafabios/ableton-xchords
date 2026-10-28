// Static download links remain usable when the API is unavailable or rate-limited.
(async () => {
  const status = document.getElementById('build-status');
  try {
    const response = await fetch('https://api.github.com/repos/rafabios/ableton-xchords/releases/tags/latest', { signal: AbortSignal.timeout(7000) });
    if (!response.ok) throw new Error('Release unavailable');
    const release = await response.json();
    const assets = release.assets || [];
    const expected = ['Xchords-Harmony-VST3-Windows-x64.zip', 'Xchords-Harmony-Standalone-Windows-x64.zip'];
    if (!expected.every(name => assets.some(asset => asset.name === name))) throw new Error('Packages pending');
    const newest = assets.filter(asset => expected.includes(asset.name)).map(asset => Date.parse(asset.updated_at)).filter(Number.isFinite);
    const date = newest.length ? new Date(Math.max(...newest)).toLocaleDateString('pt-BR') : '';
    status.textContent = `Último build disponível${date ? ' · ' + date : ''} · Windows x64 · desenvolvimento`;
  } catch {
    status.textContent = 'Consulte “Notas do último build” para disponibilidade e data. Se a primeira compilação estiver em andamento, os downloads aparecerão ao terminar.';
  }
})();
