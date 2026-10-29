#include <string.h>
#include "util.h"
#include "cadena.h"
#include "config.h"

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    opcion Salir: copia socios.dat y titulos.dat con la fecha de
                proceso en el nombre, vuelca la matriz de consumo a disco y
                libera la memoria de ambos indices.
Parametros:     ind_socios: indice de socios.
                ind_titulos: indice de titulos.
                matriz: matriz de consumo.
                fecha_proceso: fecha de proceso.
Retorno:        No posee.
Observaciones:  -
******************************************************************************/
void salir_y_persistir_ALU(t_indice *ind_socios, t_indice *ind_titulos,
                              t_matriz_consumo matriz_consumo, const t_fecha *fecha_proceso)
{
    char fecha[12], nom_arch[100];
    FILE *arch;

    sprintf (fecha, "%04d%02d%02d", fecha_proceso->anio, fecha_proceso->mes, fecha_proceso->dia);

    sprintf (nom_arch, "%s%s.dat", PREFIJO_SOCIOS_BKP, fecha);

    printf("%s -> ",ARCH_SOCIOS_BIN);

    if(archivo_copiar_ALU(ARCH_SOCIOS_BIN, nom_arch) != OK)
        printf ("(no se pudo copiar)\n");
    else
        printf("%s\n", nom_arch);

    sprintf (nom_arch, "%s%s.dat", PREFIJO_TITULOS_BKP, fecha);

    printf("%s -> ",ARCH_TITULOS_BIN);

    if(archivo_copiar_ALU(ARCH_TITULOS_BIN, nom_arch) != OK)
        printf ("(no se pudo copiar)\n");
    else
        printf("%s\n", nom_arch);

    sprintf (nom_arch, "%s%s.dat", PREFIJO_MATRIZ, fecha);

    arch = fopen(nom_arch, "wb");
    if(!arch)
        printf ("Error al abrir archivo");
    else
    {
        fwrite (matriz_consumo, sizeof(t_matriz_consumo), 1, arch);
        fclose(arch);
        printf("matriz de consumo -> %s\n",nom_arch);
    }

    indice_destruir_ALU(ind_titulos);
    indice_destruir_ALU(ind_socios);
    printf("Memoria liberada. Hasta luego.\n");
}


void carga_manual(const char *mensaje, char *cadena, const size_t tam_cad)
{
    char *salto;
    printf("%s", mensaje);
    fgets(cadena, tam_cad, stdin);
    salto = strchr(cadena, '\n');
    if(salto)
        *salto = '\0';
}

int archivo_copiar_ALU(const char *origen, const char *destino)
{
    int dato;
    FILE *p_destino, *p_origen = fopen(origen, "rb");
    if(!p_origen)
        return ERROR;
    p_destino = fopen(destino, "wb");
    if(!p_destino)
    {
        fclose(p_origen);
        return ERROR;
    }

    while((dato = fgetc(p_origen)) != EOF)
          {
              if (fputc(dato, p_destino) == EOF)
            {
                fclose(p_origen);
                fclose(p_destino);
                return ERROR;
            }
          }

    fclose(p_destino);
    fclose(p_origen);
    return OK;
}

