import sqlite3
import os

DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'papeleras.db')


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
            estado TEXT NOT NULL
        )
    ''')
    conn.commit()
    conn.close()


def insert_alerta(fecha, nivel, estado):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute(
        'INSERT INTO alertas (fecha, nivel, estado) VALUES (?, ?, ?)',
        (fecha, nivel, estado)
    )
    conn.commit()
    conn.close()


def get_alertas(limit=20):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute(
        'SELECT * FROM alertas ORDER BY id DESC LIMIT ?', (limit,)
    )
    rows = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return rows


def get_ultimo_estado():
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT estado FROM alertas ORDER BY id DESC LIMIT 1')
    row = cursor.fetchone()
    conn.close()
    return row[0] if row else None


def get_stats():
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT COUNT(*) FROM alertas')
    total = cursor.fetchone()[0]
    cursor.execute('SELECT nivel FROM alertas WHERE estado = ? ORDER BY id DESC LIMIT 1', ("alerta",))
    row = cursor.fetchone()
    max_nivel = row[0] if row else None
    conn.close()
    return {'total': total, 'max_nivel': max_nivel}
