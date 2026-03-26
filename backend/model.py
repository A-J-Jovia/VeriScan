import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, classification_report
import joblib

print("--- VERISCAN MASTER AI TRAINING ---")

# 1. Load Data
try:
    df = pd.read_csv('scans.csv')
    print(f"Loaded {len(df)} scans successfully.")
except FileNotFoundError:
    print("Error: scans.csv not found!")
    exit()

# 2. Clean the Data
# Drop the SCAN_NUM column (index 0) and the saturated channels (6, 12, 18).
# Python uses 0-based indexing, so columns 6, 12, and 18 are at indices 6, 12, and 18.
# The label is the last column (-1).
columns_to_drop = [df.columns[0], df.columns[6], df.columns[12], df.columns[18], df.columns[-1]]
X = df.drop(columns_to_drop, axis=1) 
y = df.iloc[:, -1] # The text labels

# 3. Split Data for Testing
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# 4. Train the Brain
print("\nTraining Random Forest Classifier...")
rf_model = RandomForestClassifier(n_estimators=200, max_depth=10, random_state=42)
rf_model.fit(X_train, y_train)

# 5. Test Accuracy
predictions = rf_model.predict(X_test)
accuracy = accuracy_score(y_test, predictions)

print(f"\n--- RESULTS ---")
print(f"AI Accuracy Score: {accuracy * 100:.2f}%")
print("\nClassification Report:")
print(classification_report(y_test, predictions))

# 6. Save the model
model_filename = 'veriscan_brain.pkl'
joblib.dump(rf_model, model_filename)
print(f"\nSUCCESS: Model saved as '{model_filename}'.")
print("Hand this file to the FastAPI backend!")