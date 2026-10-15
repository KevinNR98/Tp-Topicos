#include <stdlib.h>
#include <string.h>
#include "alquiler.h"
#include "cadena.h"
#include "util.h"

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    imprime por pantalla la matriz de consumo con totales por fila
                (plan) y por columna (genero).
Parametros:     m: matriz de consumo.
Retorno:        No posee.
Observaciones:  -
******************************************************************************/
void tablero_mostrar_ALU(t_matriz_consumo m)
{
    int suma = 0;
    int total = 0;
    char planes[CANT_PLANES][TAM_PLAN] = {"Basic", "Premium", "VIP", "Family"};
        printf("%-12s%10s%10s%10s%10s%18s%10s\n",
           "PLAN",
           "Accion",
           "Drama",
           "Comedia",
           "Terror",
           "Ciencia Ficcion",
           "TOTAL");
    for(int i = 0; i < CANT_PLANES; i++)
    {
        printf("%-12s", planes[i]);
        for(int j = 0; j < CANT_GENEROS; j++)
        {
            if(j == CANT_GENEROS - 1)
                printf("%18d", m[i][j]);
            else
                printf("%10d", m[i][j]);
            suma += m[i][j];
        }
        printf("%10d\n", suma);
        suma = 0;
    }
    printf("%-12s", "TOTAL");

    for(int j=0; j < CANT_GENEROS; j++)
    {
        for(int i=0; i < CANT_PLANES; i++)
        {
            suma += m[i][j];
        }
        if(j == CANT_GENEROS - 1)
            printf("%18d", suma);
        else
            printf("%10d", suma);
        total += suma;
        suma = 0;
    }

    printf("%10d\n", total);
}

/******************************************************************************
Descripcion:    ejecuta el flujo de alquiler de un titulo: 1) habilitacion del
                socio (debe estar en el indice), 2) inventario (el titulo debe
                estar en el indice y con stock > 0), 3) descuenta una unidad
                de stock en el archivo, 4) incrementa la celda [plan][genero]
                de la matriz, 5) emite el comprobante.
Parametros:     ind_socios: indice de socios.
                ind_titulos: indice de titulos.
                path_socios: ruta de socios.dat.
                path_titulos: ruta de titulos.dat.
                matriz: matriz de consumo a alimentar.
                fecha_proceso: fecha de proceso para el comprobante.
Retorno:        OK si el alquiler se concreto, ERROR si alguna etapa fallo.
Observaciones:  Si una etapa falla se informa el motivo y no se ejecutan las
                siguientes. No se exige cuota al dia.
******************************************************************************/
int alquilar_ALU(t_indice *ind_socios, t_indice *ind_titulos, const char *path_socios,
             const char *path_titulos, t_matriz_consumo matriz_consumo, const t_fecha *fecha_proceso)
{
    t_socio socio;
    t_titulo titulo;
    t_reg_indice reg_soc, reg_tit;
    char cadena[70];
    char extra;

    carga_manual("\nIngrese DNI de socio: ", cadena, sizeof(cadena));

    if(sscanf(cadena, "%ld%c", &reg_soc.id, &extra) != 1)
    {
        puts("\nAlquiler rechazado: DNI invalido.");
        return ERROR;
    }

    if(indice_buscar(ind_socios, &reg_soc, sizeof(t_reg_indice), compara_reg_indice_ALU) == NO_EXISTE)
    {
        puts("\nAlquiler rechazado: el socio no existe o esta dado de baja.");
        return ERROR;
    }

    carga_manual("\nIngrese ID del Titulo: ", cadena, sizeof(cadena));

    if(sscanf(cadena, "%ld%c", &reg_tit.id, &extra) != 1)
    {
        puts("\nAlquiler rechazado: ID invalido.");
        return ERROR;
    }

    if(indice_buscar(ind_titulos, &reg_tit, sizeof(t_reg_indice), compara_reg_indice_ALU) == NO_EXISTE)
    {
        puts("\nAlquiler rechazado: el titulo no existe o esta dado de baja.");
        return ERROR;
    }

    FILE *pf_titulos = fopen(path_titulos, "r+b");

    if(!pf_titulos)
        return ERROR;

    fseek(pf_titulos, sizeof(t_titulo) * reg_tit.nro_reg, SEEK_SET);
    fread(&titulo, sizeof(t_titulo), 1, pf_titulos);

    if(titulo.stock == 0)
    {
        printf("\nAlquiler rechazado: sin stock de \"%s\"\n", titulo.titulo);
        fclose(pf_titulos);
        return ERROR;
    }

    FILE *pf_socios = fopen(path_socios, "rb");
    if(!pf_socios)
        return ERROR;

    fseek(pf_socios, sizeof(t_socio) * reg_soc.nro_reg, SEEK_SET);
    fread(&socio, sizeof(t_socio), 1, pf_socios);
    titulo.stock--;

    puts("========== COMPROBANTE DE ALQUILER ==========");
    printf("Fecha\t: %d/%d/%d\n", fecha_proceso->dia, fecha_proceso->mes, fecha_proceso->anio);
    printf("Socio\t: %ld - %s (plan %s)\n", socio.dni, socio.apellidos_nombres, socio.plan);
    printf("Titulo\t: %d - %s (%s)\n", titulo.id, titulo.titulo, titulo.genero);
    puts("=============================================");

    sumar_alquiler_matriz(matriz_consumo, socio.plan, titulo.genero);

    fseek(pf_titulos, sizeof(t_titulo) * reg_tit.nro_reg, SEEK_SET);
    fwrite(&titulo, sizeof(t_titulo), 1, pf_titulos);

    fclose(pf_titulos);
    fclose(pf_socios);

    return OK;
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

void sumar_alquiler_matriz(int matriz[][CANT_GENEROS], const char *plan, const char *genero)
{
    if(strcmp(plan, "BASIC") == 0)
    {
        sumar_plan(matriz, 0, genero);
    }
    else if(strcmp(plan, "PREMIUM") == 0)
    {
        sumar_plan(matriz, 1, genero);
    }
    else if(strcmp(plan, "VIP") == 0)
    {
        sumar_plan(matriz, 2, genero);
    }
    else
    {
        sumar_plan(matriz, 3, genero);
    }
}

void sumar_plan(int matriz[][CANT_GENEROS], const int nro_fila, const char *genero)
{
    if(strcmp(genero, "Accion") == 0)
        matriz[nro_fila][0]++;
    else if(strcmp(genero, "Drama") == 0)
        matriz[nro_fila][1]++;
    else if(strcmp(genero, "Comedia") == 0)
        matriz[nro_fila][2]++;
    else if(strcmp(genero, "Terror") == 0)
        matriz[nro_fila][3]++;
    else
        matriz[nro_fila][4]++;
}
