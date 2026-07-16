let chartInstance = null;
let nodoActivo = 'todos';
let todosNodos = [];
let alertasCache = [];
let filasVisibles = 0;
const PAGE = 10;

const COLORS = {
    'contenedor1': { border: '#e94560', bg: 'rgba(233,69,96,0.15)', bar: '#e94560' },
    'contenedor2': { border: '#0ea5e9', bg: 'rgba(14,165,233,0.15)', bar: '#0ea5e9' },
};

function colorOf(n) {
    return COLORS[n] || { border: '#e94560', bg: 'rgba(233,69,96,0.15)', bar: '#e94560' };
}

function fmtFecha(iso) {
    const d = new Date(iso);
    return d.toLocaleDateString('es-ES', { day: '2-digit', month: '2-digit', year: '2-digit' }) + ' ' +
           d.toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}

document.getElementById('btn-add-nodo').addEventListener('click', () => {
    document.getElementById('modal-overlay').classList.remove('hidden');
    ['input-nodo-id','input-lat','input-lon','input-ubicacion'].forEach(id => document.getElementById(id).value = '');
    document.getElementById('input-nodo-id').focus();
});

function cerrarModal() {
    document.getElementById('modal-overlay').classList.add('hidden');
}

document.getElementById('btn-cancel').addEventListener('click', cerrarModal);
document.getElementById('modal-overlay').addEventListener('click', e => {
    if (e.target === e.currentTarget) cerrarModal();
});

document.getElementById('btn-confirm').addEventListener('click', async () => {
    const id = document.getElementById('input-nodo-id').value.trim();
    if (!id) return;
    const lat = parseFloat(document.getElementById('input-lat').value) || null;
    const lon = parseFloat(document.getElementById('input-lon').value) || null;
    const ubicacion = document.getElementById('input-ubicacion').value.trim() || null;
    try {
        const r = await fetch('/api/contenedores', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, lat, lon, ubicacion })
        });
        const data = await r.json();
        if (data.ok) {
            cerrarModal();
            recargarTabs();
        } else {
            alert('Error: ' + (data.error || 'desconocido'));
        }
    } catch (e) {
        alert('Error de red');
    }
});

async function recargarTabs() {
    const r = await fetch('/api/contenedores');
    const data = await r.json();
    todosNodos = data.contenedores;
    renderTabs();
    actualizar();
}

function renderTabs() {
    const tabs = document.getElementById('tabs');
    tabs.innerHTML = '<button class="tab active" data-nodo="todos">Todos</button>';
    todosNodos.forEach(n => {
        const btn = document.createElement('button');
        btn.className = 'tab';
        btn.dataset.nodo = n.id;
        btn.textContent = n.id;
        tabs.appendChild(btn);
    });
    const addBtn = document.createElement('button');
    addBtn.className = 'tab tab-add';
    addBtn.id = 'btn-add-nodo';
    addBtn.title = 'Agregar contenedor';
    addBtn.textContent = '+';
    tabs.appendChild(addBtn);
    document.getElementById('btn-add-nodo').addEventListener('click', () => {
        document.getElementById('modal-overlay').classList.remove('hidden');
        ['input-nodo-id','input-lat','input-lon','input-ubicacion'].forEach(id => document.getElementById(id).value = '');
        document.getElementById('input-nodo-id').focus();
    });
}

document.getElementById('tabs').addEventListener('click', e => {
    const btn = e.target.closest('.tab');
    if (!btn || btn.classList.contains('tab-add')) return;
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    btn.classList.add('active');
    nodoActivo = btn.dataset.nodo;
    actualizar();
});

function crearChart(datasets, labels) {
    const ctx = document.getElementById('nivelChart').getContext('2d');
    return new Chart(ctx, {
        type: 'line',
        data: { labels, datasets },
        options: {
            responsive: true,
            animation: { duration: 300 },
            interaction: { intersect: false, mode: 'index' },
            scales: {
                y: { min: 0, max: 100, grid: { color: 'rgba(255,255,255,0.05)' } },
                x: { grid: { display: false }, ticks: { maxTicksLimit: 10 } }
            },
            plugins: {
                legend: { labels: { color: '#ccc', usePointStyle: true, padding: 16 } }
            }
        },
        plugins: [{
            id: 'threshold',
            beforeDraw(chart) {
                const y = chart.scales.y.getPixelForValue(80);
                const c = chart.ctx;
                c.save();
                c.setLineDash([5, 5]);
                c.strokeStyle = 'rgba(233,69,96,0.4)';
                c.lineWidth = 1;
                c.beginPath();
                c.moveTo(chart.chartArea.left, y);
                c.lineTo(chart.chartArea.right, y);
                c.stroke();
                c.fillStyle = 'rgba(233,69,96,0.5)';
                c.font = '11px sans-serif';
                c.fillText('80% — umbral', chart.chartArea.right - 80, y - 5);
                c.restore();
            }
        }]
    });
}

function renderChart(alertas) {
    const container = document.getElementById('chart-container');
    if (alertas.length === 0) {
        container.classList.add('hidden');
        return;
    }
    container.classList.remove('hidden');

    if (nodoActivo === 'todos') {
        const datasets = todosNodos.map(n => {
            const pts = alertas.filter(a => a.nodo_id === n.id).slice().reverse();
            const c = colorOf(n.id);
            if (pts.length === 0) return null;
            return {
                label: n.id, data: pts.map(a => a.nivel),
                borderColor: c.border, backgroundColor: c.bg,
                fill: false, tension: 0.3, pointRadius: 3, borderWidth: 2
            };
        }).filter(d => d !== null);
        const labels = alertas.slice().reverse().map(a => fmtFecha(a.fecha));
        if (datasets.length === 0) { container.classList.add('hidden'); return; }
        if (chartInstance) {
            chartInstance.data.labels = labels;
            chartInstance.data.datasets = datasets;
            chartInstance.options.plugins.legend.display = true;
            chartInstance.update('none');
        } else {
            chartInstance = crearChart(datasets, labels);
        }
    } else {
        const pts = alertas.slice().reverse();
        const labels = pts.map(a => fmtFecha(a.fecha));
        const valores = pts.map(a => a.nivel);
        const c = colorOf(nodoActivo);
        const ds = [{
            label: nodoActivo, data: valores,
            segment: {
                borderColor: ctx => ctx.p0.parsed.y > 80 ? '#e94560' : '#4ecca3',
                backgroundColor: ctx => ctx.p0.parsed.y > 80 ? 'rgba(233,69,96,0.15)' : 'rgba(78,204,163,0.1)',
            },
            borderWidth: 2, fill: true, tension: 0.3,
            pointBackgroundColor: valores.map(v => v > 80 ? '#e94560' : '#4ecca3'),
            pointRadius: 4
        }];
        if (chartInstance) {
            chartInstance.data.labels = labels;
            chartInstance.data.datasets = ds;
            chartInstance.options.plugins.legend.display = false;
            chartInstance.update('none');
        } else {
            chartInstance = crearChart(ds, labels);
            chartInstance.options.plugins.legend.display = false;
        }
    }
}

function renderSummary(estado) {
    const grid = document.getElementById('summary-grid');
    if (nodoActivo !== 'todos') {
        grid.classList.add('hidden');
        return;
    }
    const cards = [];
    todosNodos.forEach(n => {
        const vivo = estado[n.id];
        const nivel = vivo ? vivo.nivel : 0;
        const estadoTxt = vivo ? vivo.estado : 'normal';
        const isAlerta = estadoTxt === 'alerta';
        const coords = (n.lat && n.lon) ? n.lat.toFixed(4) + ', ' + n.lon.toFixed(4) : null;
        const div = document.createElement('div');
        div.className = 'summary-card ' + estadoTxt;
        div.innerHTML = [
            '<div class="sc-name">' + n.id + '</div>',
            n.ubicacion ? '<div class="sc-ubicacion">' + n.ubicacion + '</div>' : '',
            '<div class="sc-nivel">' + (vivo ? nivel + '%' : '—') + '</div>',
            '<div class="sc-bar"><div class="sc-bar-fill" style="width:' + nivel + '%;background:' + (isAlerta ? '#e94560' : '#4ecca3') + '"></div></div>',
            '<div class="sc-estado">' + (vivo ? (isAlerta ? '⚠ LLENO' : '✓ Normal') : '⏳ Sin datos') + '</div>',
            coords ? '<div class="sc-coords">' + coords + '</div>' : ''
        ].join('');
        cards.push(div);
    });
    grid.replaceChildren(...cards);
    grid.classList.remove('hidden');
}

function actualizar() {
    const alertasUrl = nodoActivo === 'todos' ? '/api/alertas' : '/api/alertas?nodo=' + nodoActivo;

    Promise.all([
        fetch(alertasUrl).then(r => r.json()),
        fetch('/api/estado').then(r => r.json())
    ]).then(([data, estado]) => {
            document.getElementById('last-update').textContent = 'Actualizado: ' + new Date().toLocaleTimeString('es-ES');

            alertasCache = data.alertas;
            filasVisibles = PAGE;
            const nodoInfo = Object.fromEntries(todosNodos.map(n => [n.id, n]));
            const tbody = document.getElementById('alertas-body');
            const rows = alertasCache.slice(0, filasVisibles).map(a => {
                const c = colorOf(a.nodo_id);
                const info = nodoInfo[a.nodo_id];
                const tr = document.createElement('tr');
                tr.className = a.estado === 'alerta' ? 'alerta' : 'normal';
                tr.style.borderLeft = '3px solid ' + c.border;
                const pct = a.nivel;
                const barColor = pct > 80 ? '#e94560' : '#4ecca3';
                tr.innerHTML = [
                    '<td>' + a.id + '</td>',
                    '<td><span class="nodo-badge" style="background:' + c.border + '22;color:' + c.border + ';border:1px solid ' + c.border + '44">' + a.nodo_id + '</span></td>',
                    '<td>' + (info ? info.ubicacion || (info.lat && info.lon ? info.lat.toFixed(4) + ', ' + info.lon.toFixed(4) : '—') : '—') + '</td>',
                    '<td>' + fmtFecha(a.fecha) + '</td>',
                    '<td><div class="nivel-cell"><div class="nivel-bar"><div class="nivel-bar-fill" style="width:' + pct + '%;background:' + barColor + '"></div></div><span class="nivel-text">' + pct + '%</span></div></td>',
                    '<td><span class="estado-badge ' + a.estado + '">' + (a.estado === 'alerta' ? '⚠ Alerta' : '✓ Normal') + '</span></td>'
                ].join('');
                return tr;
            });
            tbody.replaceChildren(...rows);

            document.getElementById('stat-total').textContent = data.stats.total;
            const stat2 = document.getElementById('stat-max');
            const stat2Label = document.getElementById('stat2-label');
            if (nodoActivo === 'todos') {
                stat2Label.textContent = 'Contenedores activos';
                stat2.textContent = Object.keys(estado).length;
            } else {
                stat2Label.textContent = 'Último nivel máximo';
                stat2.textContent = (data.stats.max_nivel ?? 0) + '%';
            }

            const banner = document.getElementById('status-banner');
            const locSpan = document.getElementById('status-location');
            if (nodoActivo === 'todos') {
                banner.classList.add('hidden');
            } else {
                banner.classList.remove('hidden');
                const icon = document.getElementById('status-icon');
                const text = document.getElementById('status-text');
                const cont = todosNodos.find(n => n.id === nodoActivo);
                const locParts = [];
                if (cont) {
                    if (cont.ubicacion) locParts.push(cont.ubicacion);
                    if (cont.lat != null && cont.lon != null) locParts.push(cont.lat.toFixed(4) + ', ' + cont.lon.toFixed(4));
                }
                locSpan.textContent = locParts.length ? '📍 ' + locParts.join(' · ') : '';
                const vivo = estado[nodoActivo];
                if (vivo) {
                    if (vivo.estado === 'alerta') {
                        banner.className = 'status-alerta';
                        icon.textContent = '⚠';
                        text.textContent = nodoActivo + ': Contenedor lleno (' + vivo.nivel + '%)';
                    } else {
                        banner.className = 'status-normal';
                        icon.textContent = '✓';
                        text.textContent = nodoActivo + ': Contenedor normal (' + vivo.nivel + '%)';
                    }
                } else {
                    const ultimo = data.alertas[0];
                    if (ultimo) {
                        if (ultimo.estado === 'alerta') {
                            banner.className = 'status-alerta';
                            icon.textContent = '⚠';
                            text.textContent = nodoActivo + ': ' + ultimo.nivel + '% (último registro)';
                        } else {
                            banner.className = 'status-normal';
                            icon.textContent = '✓';
                            text.textContent = nodoActivo + ': ' + ultimo.nivel + '% (último registro)';
                        }
                    } else {
                        banner.className = 'status-normal';
                        icon.textContent = '⏳';
                        text.textContent = nodoActivo + ': Sin datos';
                    }
                }
            }

            const btnVerMas = document.getElementById('btn-ver-mas');
            btnVerMas.classList.toggle('hidden', alertasCache.length <= filasVisibles);

            renderSummary(estado);
            renderChart(data.alertas);
        })
        .catch(err => console.error('Error:', err));
}

function verMas() {
    filasVisibles += PAGE;
    const tbody = document.getElementById('alertas-body');
    const rows = alertasCache.slice(0, filasVisibles).map(a => {
        const c = colorOf(a.nodo_id);
        const info = Object.fromEntries(todosNodos.map(n => [n.id, n]))[a.nodo_id];
        const tr = document.createElement('tr');
        tr.className = a.estado === 'alerta' ? 'alerta' : 'normal';
        tr.style.borderLeft = '3px solid ' + c.border;
        const pct = a.nivel;
        const barColor = pct > 80 ? '#e94560' : '#4ecca3';
        tr.innerHTML = [
            '<td>' + a.id + '</td>',
            '<td><span class="nodo-badge" style="background:' + c.border + '22;color:' + c.border + ';border:1px solid ' + c.border + '44">' + a.nodo_id + '</span></td>',
            '<td>' + (info ? info.ubicacion || (info.lat && info.lon ? info.lat.toFixed(4) + ', ' + info.lon.toFixed(4) : '—') : '—') + '</td>',
            '<td>' + fmtFecha(a.fecha) + '</td>',
            '<td><div class="nivel-cell"><div class="nivel-bar"><div class="nivel-bar-fill" style="width:' + pct + '%;background:' + barColor + '"></div></div><span class="nivel-text">' + pct + '%</span></div></td>',
            '<td><span class="estado-badge ' + a.estado + '">' + (a.estado === 'alerta' ? '⚠ Alerta' : '✓ Normal') + '</span></td>'
        ].join('');
        return tr;
    });
    tbody.replaceChildren(...rows);
    document.getElementById('btn-ver-mas').classList.toggle('hidden', alertasCache.length <= filasVisibles);
}
