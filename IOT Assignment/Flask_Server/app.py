from flask import Flask, render_template, request, jsonify

app = Flask(__name__)

# Global State
system_state = {
    "light": 0.00,
    "pattern": "blink",
    # For live light graph 
    "history": [] 
}

@app.route('/')
def index():
    # Serves the responsive web interface 
    return render_template('index.html')

@app.route('/api/data', methods=['GET', 'POST'])
def handle_data():
    if request.method == 'POST':
        data = request.json
        # Look for light key sent by the ESP32 
        system_state["light"] = f"{float(data.get('light', 0)):.2f}"
        
        # Keep history to 20 data points  
        system_state["history"].append(system_state["light"])
        if len(system_state["history"]) > 20:
            system_state["history"].pop(0)
            
        return jsonify({"status": "success"})
    
    # Return light data for AJAX updates on the web page 
    return jsonify({
        "light": system_state["light"],
        "history": system_state["history"],
        "pattern": system_state["pattern"]
    })

@app.route('/api/pattern', methods=['GET', 'POST'])
def handle_pattern():
    if request.method == 'POST':
        data = request.json
        # Stores current led pattern selected  
        system_state["pattern"] = data.get("pattern", "blink")
        return jsonify({"status": "success"})
    
    return system_state["pattern"]

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5002, debug=True)