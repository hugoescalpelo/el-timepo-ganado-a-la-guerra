import cv2
import numpy as np

def calcular_area_negra(imagen_path, ancho_cm, largo_cm):
    # Cargar la imagen en escala de grises
    imagen = cv2.imread(imagen_path, cv2.IMREAD_GRAYSCALE)
    
    if imagen is None:
        raise ValueError("No se pudo cargar la imagen. Verifique la ruta y el formato.")

    # Aplicar umbral para segmentar las áreas negras
    _, imagen_binaria = cv2.threshold(imagen, 127, 255, cv2.THRESH_BINARY)

    # Contar los píxeles negros
    pixeles_negros = np.sum(imagen_binaria == 0)

    # Convertir el área de píxeles a centímetros cuadrados
    alto_pixeles, ancho_pixeles = imagen_binaria.shape
    pixel_por_cm_ancho = ancho_pixeles / ancho_cm
    pixel_por_cm_largo = alto_pixeles / largo_cm

    area_cm2 = pixeles_negros / (pixel_por_cm_ancho * pixel_por_cm_largo)
    
    return area_cm2

# Ejemplo de uso
ruta_imagen = r'C:\\Users\\hugoe\\Documents\\GitHub\\el-timepo-ganado-a-la-guerra\\PNG\\carta-22-superior-ajustado.png'  # Cambia esto por la ruta real de tu imagen
ancho_cm = 25  # Ancho del rectángulo en cm
largo_cm = 90   # Largo del rectángulo en cm

area = calcular_area_negra(ruta_imagen, ancho_cm, largo_cm)
print(f"El área de la zona negra es: {area:.2f} cm²")
