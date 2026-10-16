#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auditoria.h"
#include "lectura.h"
#include "cadena.h"
#include "util.h"
#include "indice.h"

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    agrega al final del archivo de auditoria una linea con el
                formato DNI;TIPO_VALIDACION;FECHA_PROCESO.
Parametros:     path: ruta del archivo de texto de errores.
                dni_texto: DNI tal como vino en la entrada.
                tipo_error: nombre de la validacion que fallo.
                fecha_proceso: fecha de proceso.
Retorno:        OK si se pudo grabar, ERROR en caso contrario.
Observaciones:  El archivo se abre en modo append; si no existe se crea.
******************************************************************************/
int auditoria_registrar_ALU(const char *path, const char *dni_texto,
                        const char *tipo_error, const t_fecha *fecha_proceso)
{
    FILE *pf = fopen(path, "at");
    if (!pf)
        return ERROR;

    fprintf(pf, "%s;%s;%02d/%02d/%04d\n", dni_texto, tipo_error, fecha_proceso->dia, fecha_proceso->mes, fecha_proceso->anio);

    fclose(pf);
    return OK;
    //return auditoria_registrar(path, dni_texto, tipo_error, fecha_proceso);
}

/******************************************************************************
Descripcion:    lee el archivo de auditoria y muestra, por cada par (DNI, tipo
                de validacion), la cantidad de incidencias registradas y el
                total general.
Parametros:     path: ruta del archivo de texto de errores.
Retorno:        OK si se pudo procesar (aunque el archivo no exista), ERROR si
                fallo la memoria.
Observaciones:  Acumula en un vector dinamico que crece con realloc.
******************************************************************************/
int auditoria_mostrar_ALU(const char *path)
{
    FILE *pf = fopen(path, "rt");
    if (!pf)
    {
        printf("El archivo de auditoria no existe o esta vacio. No hay errores registrados.\n");
        return OK;
    }

    int capacidad = 10;
    int cantidad_elementos = 0;
    t_auditoria *vec = malloc(capacidad * sizeof(t_auditoria));
    if (!vec)
    {
        fclose(pf);
        return ERROR;
    }

    char linea[TAM_LINEA];
    int total_general = 0;

    while (fgets(linea, sizeof(linea), pf))
    {
        char dni[TAM_DNI];
        char tipo_error[TAM_TIPO_ERROR];

        if (sscanf(linea, "%[^;];%[^;\n]", dni, tipo_error) == 2)
        {
            int encontrado = 0;

            for (int i = 0; i < cantidad_elementos; i++)
            {
                if (strcmp(vec[i].dni, dni) == 0 && strcmp(vec[i].tipo_error, tipo_error) == 0)
                {
                    vec[i].cantidad++;
                    encontrado = 1;
                    break;
                }
            }

            if (!encontrado)
            {
                if (cantidad_elementos >= capacidad)
                {
                    capacidad *= INCREMENTO; // Aumenta 30%
                    t_auditoria *aux = realloc(vec, capacidad * sizeof(t_auditoria));
                    if (!aux)
                    {
                        free(vec);
                        fclose(pf);
                        return ERROR;
                    }
                    vec = aux;
                }

                strcpy(vec[cantidad_elementos].dni, dni);
                strcpy(vec[cantidad_elementos].tipo_error, tipo_error);
                vec[cantidad_elementos].cantidad = 1;
                cantidad_elementos++;
            }

            total_general++;
        }
    }

    fclose(pf);

    printf("%-20s | %-25s | %10s\n", "DNI", "TIPO DE VALIDACION", "CANTIDAD");
    printf("--------------------------------------------------------------------\n");

    for (int i = 0; i < cantidad_elementos; i++)
    {
        printf("%-20s | %-25s | %10d\n", vec[i].dni, vec[i].tipo_error, vec[i].cantidad);
    }


    printf("\nTotal de incidencias: %d\n", total_general);

    free(vec);
    return OK;
    //return auditoria_mostrar(path);
}
