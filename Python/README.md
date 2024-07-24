# Python

En este documento encontrarás las instrucciones para correr los programas necesarios en windows. Las instrucciones para correrlo en Linux son similares.

## Instalar Python

Descarga la última versión de Python desde el sitio oficial [https://www.python.org/downloads/](https://www.python.org/downloads/). Este programa fue probado con la versión 3.12.4

Asegurate de agregar la ruta de instalación de Python al Path del sistema.

Comprueba la versión con una terminal y el siguiente comando.
```
python --version
```

Actualiza PIP con el siguiente comando

```
python.exe -m pip install --upgrade pip
```
## Instala las bibliotecas necesarias

Instala las bibliotecas necesarias con el siguiente comando.

```
pip install opencv-python numpy
```

## Visual Studio Code

Este programa fue ejecutado con Visual Studio Code, puedes descargarlo de su sitio oficial [https://code.visualstudio.com/](https://code.visualstudio.com/)

Es necesario instalar la extensión de Python para Visual Studio Code.

## area.py

Este programa calcula el area de una zona negra en una imagen en fondo blanco. Se usa para calcular el area de las aperturas de los cajones resonadores, la cual es necesaria para hacer el calculo de la longitud del cuello del resonador de Helholtz y lograr escoger la frecuencia de resonancia.

Requiere contar con una imagen en .png que tenga colreada de negro el area de la apertura.

Para ejecutarlo, es necesario configurar la ruta de la imagen y las dimensiones del area de la placa general, las cuales se encuentran cerca del final del programa.