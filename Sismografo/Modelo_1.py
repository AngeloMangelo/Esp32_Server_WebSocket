import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report, confusion_matrix
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

# === 1. Cargar y preparar datos ===
df = pd.read_csv('Data.csv')
df = df.loc[:, ~df.columns.str.contains('^Unnamed')]
df.dropna(inplace=True)

# === 2. Crear nuevas características ===
df['a_magnitude'] = (df['aX']**2 + df['aY']**2 + df['aZ']**2)**0.5
df['g_magnitude'] = (df['gX']**2 + df['gY']**2 + df['gMZ']**2)**0.5
df['abs_gX'] = abs(df['gX'])
df['abs_aZ'] = abs(df['aZ'])

# === 3. Seleccionar variables predictoras y target ===
features = ['aX', 'aY', 'aZ', 'gX', 'gY', 'gMZ', 'a_magnitude', 'g_magnitude', 'abs_gX', 'abs_aZ']
X = df[features]
y = df['Result']

# === 4. Normalización y división ===
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

X_train, X_test, y_train, y_test = train_test_split(X_scaled, y, test_size=0.25, random_state=42)

# === 5. Entrenamiento ===
clf = RandomForestClassifier(n_estimators=100, random_state=42)
clf.fit(X_train, y_train)

# === 6. Evaluación ===
y_pred = clf.predict(X_test)

print("=== Matriz de confusión ===")
print(confusion_matrix(y_test, y_pred))
print("\n=== Reporte de clasificación ===")
print(classification_report(y_test, y_pred))

# === 7. Importancia de características ===
importances = clf.feature_importances_
feat_imp = pd.Series(importances, index=features).sort_values(ascending=False)

plt.figure(figsize=(10, 5))
sns.barplot(x=feat_imp.values, y=feat_imp.index, palette='viridis')
plt.title('Importancia de las características')
plt.xlabel('Importancia')
plt.grid(True)
plt.tight_layout()
plt.show()

import joblib

# Guardar el modelo entrenado
joblib.dump(clf, 'modelo_sismo.pkl')
print("Modelo guardado como modelo_sismo.pkl")

# Guardar también el scaler si lo usaste (muy recomendable)
joblib.dump(scaler, 'escalador.pkl')
print("Scaler guardado como escalador.pkl")
