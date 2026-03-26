from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
import pandas as pd
import joblib

print("Booting VeriScan Inference Engine...")

# 1. Load the Entire Pipeline (Scaler + Brain)
try:
    model_pipeline = joblib.load('veriscan_pipeline.pkl')
    print("Pipeline loaded successfully.")
except FileNotFoundError:
    raise RuntimeError("veriscan_pipeline.pkl missing. Run model.py first.")

# 2. Start the API
app = FastAPI(title="VeriScan API", version="2.0")

# 3. Strict Input Validation via Pydantic
class ScanData(BaseModel):
    # We force the Flutter app to send exactly 18 integers. 
    # If they send 17 or 19, the API rejects it automatically with a 422 error.
    channels: list[int] = Field(..., min_length=18, max_length=18)

@app.post("/predict")
def predict_pill(data: ScanData):
    try:
        # Convert the 18 incoming integers into a 1-row DataFrame 
        # with the exact column names expected by the cleaning logic.
        columns = [f'CH{i}' for i in range(1, 19)]
        df = pd.DataFrame([data.channels], columns=columns)

        # Drop the 3 saturated channels to match the 15-channel pipeline
        df_clean = df.drop(columns=['CH6', 'CH12', 'CH18'])

        # The pipeline automatically applies the L1 Normalizer to this single row, 
        # then passes it to the Random Forest.
        prediction = model_pipeline.predict(df_clean)[0]

        return {
            "status": "success",
            "verdict": str(prediction)
        }

    except Exception as e:
        # Catch any unexpected Pandas/Scikit-Learn errors so the server doesn't crash
        raise HTTPException(status_code=500, detail=f"Inference failed: {str(e)}")

@app.get("/health")
def health_check():
    return {"status": "active", "model": "veriscan_pipeline.pkl"}