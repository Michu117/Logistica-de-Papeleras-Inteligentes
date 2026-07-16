import sqlite3
import os

DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'contenedores.db')


def get_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS alertas(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            fecha TEXT NOT NULL,
            nivel INTEGER NOT NULL,
            estado TEXT NOT NULL,
            nodo_id TEXT NOT NULL DEFAULT 'nodo1'
        )
    ''')
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS contenedores(
            id TEXT PRIMARY KEY,
            lat REAL,
            lon REAL,
            ubicacion TEXT
        )
    ''')
    conn.commit()
    conn.close()


def insert_alerta(fecha, nivel, estado, nodo_id='nodo1'):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute(
        'INSERT INTO alertas (fecha, nivel, estado, nodo_id) VALUES (?, ?, ?, ?)',
        (fecha, nivel, estado, nodo_id)
    )
    conn.commit()
    conn.close()


def get_alertas(limit=20, nodo_id=None):
    conn = get_connection()
    cursor = conn.cursor()
    if nodo_id:
        cursor.execute(
            'SELECT * FROM alertas WHERE nodo_id = ? ORDER BY id DESC LIMIT ?',
            (nodo_id, limit)
        )
    else:
        cursor.execute(
            'SELECT * FROM alertas ORDER BY id DESC LIMIT ?', (limit,)
        )
    rows = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return rows


def get_ultimo_estado(nodo_id=None):
    conn = get_connection()
    cursor = conn.cursor()
    if nodo_id:
        cursor.execute(
            'SELECT estado FROM alertas WHERE nodo_id = ? ORDER BY id DESC LIMIT 1',
            (nodo_id,)
        )
    else:
        cursor.execute('SELECT estado FROM alertas ORDER BY id DESC LIMIT 1')
    row = cursor.fetchone()
    conn.close()
    return row[0] if row else None


def get_stats(nodo_id=None):
    conn = get_connection()
    cursor = conn.cursor()
    if nodo_id:
        cursor.execute(
            'SELECT COUNT(*) FROM alertas WHERE nivel > 80 AND nodo_id = ?', (nodo_id,)
        )
        total = cursor.fetchone()[0]
        cursor.execute(
            'SELECT nivel FROM alertas WHERE estado = ? AND nodo_id = ? ORDER BY id DESC LIMIT 1',
            ("alerta", nodo_id)
        )
    else:
        cursor.execute('SELECT COUNT(*) FROM alertas WHERE nivel > 80')
        total = cursor.fetchone()[0]
        cursor.execute(
            'SELECT nivel FROM alertas WHERE estado = ? ORDER BY id DESC LIMIT 1',
            ("alerta",)
        )
    row = cursor.fetchone()
    max_nivel = row[0] if row else None
    conn.close()
    return {'total': total, 'max_nivel': max_nivel}


def get_nodos():
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM contenedores ORDER BY id')
    rows = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return rows


def add_nodo(nodo_id, lat=None, lon=None, ubicacion=None):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute(
            'INSERT OR IGNORE INTO contenedores (id, lat, lon, ubicacion) VALUES (?, ?, ?, ?)',
            (nodo_id, lat, lon, ubicacion)
        )
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()


def update_nodo(nodo_id, lat=None, lon=None, ubicacion=None):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute(
            'UPDATE contenedores SET lat = ?, lon = ?, ubicacion = ? WHERE id = ?',
            (lat, lon, ubicacion, nodo_id)
        )
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()
