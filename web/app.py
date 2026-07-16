from flask import Flask, jsonify, render_template, request
import threading
import database
import mqtt_client

app = Flask(__name__)


@app.route('/')
def dashboard():
    nodos = database.get_nodos()
    stats = database.get_stats()
    alertas = database.get_alertas(50)
    return render_template('dashboard.html', stats=stats, alertas=alertas, nodos=nodos)


@app.route('/api/alertas')
def api_alertas():
    nodo_id = request.args.get('nodo')
    alertas = database.get_alertas(50, nodo_id=nodo_id)
    stats = database.get_stats(nodo_id=nodo_id)
    return jsonify({'alertas': alertas, 'stats': stats})


@app.route('/api/estado')
def api_estado():
    return jsonify(mqtt_client.lecturas_en_vivo)


@app.route('/api/contenedores', methods=['GET', 'POST'])
def api_contenedores():
    if request.method == 'POST':
        data = request.get_json()
        nodo_id = data.get('id', '').strip().replace(' ', '')
        if not nodo_id:
            return jsonify({'error': 'ID requerido'}), 400
        if not nodo_id.startswith('contenedor'):
            return jsonify({'error': 'ID debe empezar con contenedor'}), 400
        database.add_nodo(nodo_id, data.get('lat'), data.get('lon'), data.get('ubicacion'))
        return jsonify({'ok': True, 'id': nodo_id})
    nodos = database.get_nodos()
    return jsonify({'contenedores': nodos})


@app.route('/api/contenedores/<nodo_id>', methods=['PUT'])
def api_update_nodo(nodo_id):
    data = request.get_json()
    database.update_nodo(nodo_id, data.get('lat'), data.get('lon'), data.get('ubicacion'))
    return jsonify({'ok': True})


if __name__ == '__main__':
    database.init_db()
    mqtt_thread = threading.Thread(
        target=mqtt_client.start_client, daemon=True
    )
    mqtt_thread.start()
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)
