import joblib
import numpy as np

# Cargar modelo y scaler
modelo = joblib.load('modelo_sismo.pkl')
scaler = joblib.load('escalador.pkl')

# === Simular una nueva medición de sensores ===
# aX, aY, aZ, gX, gY, gMZ, abs_az, abs_gx, a_magnitude, g_magnitude
nueva_muestra = np.array([[-584, -1416, -17096, -3536, 58, 280, 17164.47867, 3547.542812, 17096, 3536]])

# Escalar la muestra
nueva_muestra_esc = scaler.transform(nueva_muestra)

# Predecir
resultado = modelo.predict(nueva_muestra_esc)

print("Predicción:", "Sismo" if resultado[0] == 1 else "No sismo")
