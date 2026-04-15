from flask import Flask, render_template, request, jsonify

app = Flask(__name__)

# Global State - Renamed 'temperature' to 'light' for clarity 
system_state = {
    "light": 0.0,
    "pattern": "blink",
    "history": [] # For live graph extra credit 
}

@app.route('/')
def index():
    # Serves the responsive web interface 
    return render_template('index.html')

@app.route('/api/data', methods=['GET', 'POST'])
def handle_data():
    if request.method == 'POST':
        data = request.json
        # Look for the 'light' key sent by the ESP32 
        system_state["light"] = round(data.get("light", 0), 2)
        
        # Keep history to 20 data points for the live graph enhancement 
        system_state["history"].append(system_state["light"])
        if len(system_state["history"]) > 20:
            system_state["history"].pop(0)
            
        return jsonify({"status": "success"})
    
    # Return light data for the AJAX updates on the web page 
    return jsonify({
        "light": system_state["light"],
        "history": system_state["history"],
        "pattern": system_state["pattern"]
    })

@app.route('/api/pattern', methods=['GET', 'POST'])
def handle_pattern():
    if request.method == 'POST':
        data = request.json
        # Stores the current LED pattern selected by the user 
        system_state["pattern"] = data.get("pattern", "blink")
        return jsonify({"status": "success"})
    
    # The ESP32 performs a GET request here to see which pattern to run 
    return system_state["pattern"]

if __name__ == '__main__':
    # host '0.0.0.0' allows the ESP32 to connect via your local Wi-Fi 
    app.run(host='0.0.0.0', port=5002, debug=True)