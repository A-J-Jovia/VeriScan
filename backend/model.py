import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import Normalizer
from sklearn.pipeline import Pipeline
from sklearn.metrics import accuracy_score, classification_report
import joblib

print("--- VERISCAN MASTER PIPELINE TRAINING v2 ---")

# 1. Load Data
# (Using  current scans.csv. Will update this filename after gather V2 datavfrom the updated hardware)
try:
    df = pd.read_csv('scans.csv')
    print(f"Loaded {len(df)} raw scans.")
except FileNotFoundError:
    print("FATAL: scans.csv not found! Place it in the backend/ directory.")
    exit()

# 2. Clean Data (Strict Column Dropping)
# We drop SCAN_NUM (useless for ML) and the 3 saturated channels.
# Dropped CH18 which is nir for now. Will add it back once hardware is tuned 
columns_to_drop = ['SCAN_NUM', 'CH6', 'CH12', 'CH18']

# Safely drop only the columns that actually exist in the dataframe
X = df.drop(columns=[col for col in columns_to_drop if col in df.columns])

# Extract the labels (target variable)
# Ensure we don't accidentally leave the LABEL column in the training features (X)
target_col = 'LABEL'
if target_col in X.columns:
    y = X[target_col]
    X = X.drop(columns=[target_col])
else:
    # Fallback if the column is not explicitly named 'LABEL'
    y = df.iloc[:, -1]
    X = X.iloc[:, :-1]

print(f"Features mapped: {len(X.columns)} channels remaining after cleaning.")

# 3. Train/Test Split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# 4. Row-wise L1 Normalization -> Random Forest
print("Training L1-Normalized Random Forest Pipeline...")
pipeline = Pipeline([
    # norm='l1' forces the sum of the 15 channels to equal 1.0 (100%)
    # This extracts the chemical ratio and eliminates physical distance variance.
    ('scaler', Normalizer(norm='l1')), 
    ('classifier', RandomForestClassifier(n_estimators=200, max_depth=10, random_state=42))
])

pipeline.fit(X_train, y_train)

# 5. Scientific Validation
y_pred = pipeline.predict(X_test)
print(f"\nValidation Accuracy: {accuracy_score(y_test, y_pred) * 100:.2f}%\n")
print(classification_report(y_test, y_pred))

# 6. Save the ENTIRE Pipeline
joblib.dump(pipeline, 'veriscan_pipeline.pkl')
print("\n[SUCCESS] Pipeline saved as veriscan_pipeline.pkl")