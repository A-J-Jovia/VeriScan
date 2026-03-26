from fastapi import FastAPI
from pydantic import BaseModel
import pandas as pd
import joblib
import uvicorn

# 1. Load the Brain
model = joblib.load('veriscan_brain.pkl')

# 2. Start the API
app = FastAPI(title="VeriScan AI API")

# 3. Define the incoming data from the Flutter App
class ScanData(BaseModel):
    channels: list[int] # Expecting exactly 18 numbers from the ESP32

@app.post("/predict")
def predict_pill(data: ScanData):
    # Check if we got exactly 18 channels
    if len(data.channels) != 18:
        return {"error": "Expected exactly 18 channels of data."}

    # Convert the 18 numbers into a DataFrame with the exact column names the AI learned
    columns = [f'CH{i}' for i in range(1, 19)]
    df = pd.DataFrame([data.channels], columns=columns)

    # Drop the 3 saturated channels (CH6, CH12, CH18) just like we did in training
    df_clean = df.drop(columns=['CH6', 'CH12', 'CH18'])

    # Ask the AI to make a prediction!
    prediction = model.predict(df_clean)[0]

    # Return the verdict to the Flutter app
    return {
        "status": "success",
        "verdict": prediction
    }

print("--- VERISCAN API READY ---")
print("Tell Faz to send a POST request to: http://localhost:8000/predict")

# Run the server (Only if this file is run directly)
if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)