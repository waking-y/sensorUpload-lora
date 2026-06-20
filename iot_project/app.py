import sqlite3
import json
import threading
from flask import Flask, jsonify, render_template
import paho.mqtt.client as mqtt

# ================= 1. 数据库操作逻辑 =================
DB_NAME = "sensor_data.db"

def init_db():
    """初始化 SQLite 数据库和表"""
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT (datetime('now', 'localtime')),
            temp REAL,
            humi REAL,
            lat REAL,
            lon REAL,
            sats INTEGER
        )
    ''')
    conn.commit()
    conn.close()

def insert_data(temp, humi, lat, lon, sats):
    """插入一条新数据"""
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    c.execute('''
        INSERT INTO sensor_data (temp, humi, lat, lon, sats) 
        VALUES (?, ?, ?, ?, ?)
    ''', (temp, humi, lat, lon, sats))
    conn.commit()
    conn.close()

# ================= 2. MQTT 接收服务 =================
def on_connect(client, userdata, flags, rc):
    print("MQTT Connected with result code " + str(rc))
    client.subscribe("hardware/dht11")

def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    print(f"MQTT Received: {payload}")
    try:
        data = json.loads(payload)
        # 提取数据，如果某些字段缺失，设置默认值为 0
        temp = data.get('temp', 0)
        humi = data.get('humi', 0)
        lat = data.get('lat', 0.0)
        lon = data.get('lon', 0.0)
        sats = data.get('sats', 0)
        
        # 保存到数据库
        insert_data(temp, humi, lat, lon, sats)
    except Exception as e:
        print(f"Data parse error: {e}")

def mqtt_thread():
    """后台运行 MQTT 监听的线程函数"""
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    # 连接到本机运行的 Mosquitto
    client.connect("localhost", 1883, 60)
    client.loop_forever()

# ================= 3. Web 接口与服务 (Flask) =================
app = Flask(__name__)

@app.route('/')
def index():
    """访问主页时，渲染 templates/index.html 前端页面"""
    return render_template('index.html')

@app.route('/api/data')
def get_data():
    """前端 AJAX 请求该接口获取最新的 20 条数据"""
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    # 按倒序查出最新20条，然后再翻转为正序供折线图显示
    c.execute("SELECT timestamp, temp, humi, lat, lon, sats FROM sensor_data ORDER BY id DESC LIMIT 20")
    rows = c.fetchall()
    conn.close()
    
    result = []
    # [::-1] 实现了列表反转
    for row in rows[::-1]:
        result.append({
            "time": row[0],
            "temp": row[1],
            "humi": row[2],
            "lat": row[3],
            "lon": row[4],
            "sats": row[5]
        })
    return jsonify(result)

if __name__ == '__main__':
    # 1. 建库建表
    init_db()
    
    # 2. 启动 MQTT 监听线程 (守护线程)
    t = threading.Thread(target=mqtt_thread, daemon=True)
    t.start()
    
    print("\n🚀 后端服务已启动！请在浏览器访问: http://你的服务器公网IP:8080\n")
    
    # 3. 启动 Flask Web 服务器，监听 8080 端口
    app.run(host='0.0.0.0', port=8080)