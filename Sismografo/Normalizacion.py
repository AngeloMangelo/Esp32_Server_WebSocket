import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn.preprocessing import StandardScaler
from sklearn.decomposition import PCA

# === 1. Cargar y preparar datos ===
df = pd.read_csv('Data.csv')

# Eliminar columnas 'Unnamed' que no contienen información útil
df = df.loc[:, ~df.columns.str.contains('^Unnamed')]

# Verifica columnas y valores nulos
print("Primeras filas del dataset:")
print(df.head())
print("\nConteo por clase (Sismo vs No sismo):")
print(df['Result'].value_counts())

# Limpieza de datos
print("\nRevisando valores nulos...")
print(df.isnull().sum())
df.dropna(inplace=True)

# Asegúrate de que todas las columnas necesarias están
required_cols = ['aX', 'aY', 'aZ', 'gX', 'gY', 'gMZ']
assert all(col in df.columns for col in required_cols), "Faltan columnas necesarias."

# === 2. Crear nuevas características ===
df['a_magnitude'] = np.sqrt(df['aX']**2 + df['aY']**2 + df['aZ']**2)
df['g_magnitude'] = np.sqrt(df['gX']**2 + df['gY']**2 + df['gMZ']**2)
df['abs_aZ'] = np.abs(df['aZ'])
df['abs_gX'] = np.abs(df['gX'])

# Actualizar lista de columnas para análisis
feature_cols = required_cols + ['a_magnitude', 'g_magnitude', 'abs_aZ', 'abs_gX']
X = df[feature_cols]
y = df['Result']

# === 3. Escalar variables ===
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

# === 4. Nuevo mapa de correlación con transformaciones ===
plt.figure(figsize=(12, 8))
corr = df[feature_cols + ['Result']].corr()
sns.heatmap(corr, annot=True, cmap='coolwarm', fmt=".2f")
plt.title("Mapa de correlación actualizado con transformaciones")
plt.show()

# === 5. Distribución KDE por variable ===
for col in feature_cols:
    plt.figure(figsize=(7, 4))
    sns.kdeplot(data=df, x=col, hue='Result', fill=True, palette='Set2')
    plt.title(f'Distribución de {col} según clase')
    plt.grid(True)
    plt.show()

# === 6. Análisis PCA (Visualización 2D) con nuevas variables ===
pca = PCA(n_components=2)
X_pca = pca.fit_transform(X_scaled)

# Mostrar cuánta varianza explica cada componente
explained_var = pca.explained_variance_ratio_
print(f"\nVarianza explicada por PCA: {explained_var[0]:.2f}, {explained_var[1]:.2f}")

# Visualización
plt.figure(figsize=(8, 6))
sns.scatterplot(x=X_pca[:, 0], y=X_pca[:, 1], hue=y, palette='Set1', alpha=0.6)
plt.title('PCA - Visualización 2D con transformaciones')
plt.xlabel('Componente Principal 1')
plt.ylabel('Componente Principal 2')
plt.grid(True)
plt.show()

df.to_csv('Data_transformado.csv', index=False)
print("✅ Dataset guardado como 'Data_transformado.csv'")


