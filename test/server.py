from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/test', methods=['POST'])
def test():
    data = request.json  # 获取JSON格式的数据
    return jsonify({'received_data': data}), 200

if __name__ == '__main__':
    app.run(debug=True)