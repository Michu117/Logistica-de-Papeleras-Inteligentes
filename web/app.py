from flask import Flask, jsonify, render_template
import threading
import database
import mqtt_client

app = Flask(__name__)


@app.route('/')
def dashboard():
    stats = database.get_stats()
    alertas = database.get_alertas(20)
    return render_template('dashboard.html', stats=stats, alertas=alertas)


@app.route('/api/alertas')
def api_alertas():
    alertas = database.get_alertas(50)
    stats = database.get_stats()
    return jsonify({'alertas': alertas, 'stats': stats})


if __name__ == '__main__':
    database.init_db()
    mqtt_thread = threading.Thread(
        target=mqtt_client.start_client, daemon=True
    )
    mqtt_thread.start()
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)
