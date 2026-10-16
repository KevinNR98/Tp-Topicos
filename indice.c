#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "indice.h"

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    toma memoria para nmemb elementos e inicializa la estructura
                vacia.
Parametros:     indice: TDA indice.
                nmemb: cantidad inicial de elementos (CANTIDAD_ELEMENTOS).
                tamanyo: espacio en bytes ocupado por cada elemento.
Retorno:        OK si la operacion fue exitosa y ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int indice_crear_ALU(t_indice *indice, size_t nmemb, size_t tamanyo)
{
    indice->vindice = malloc(nmemb * tamanyo);
    if (indice->vindice == NULL)
    {
        indice->cantidad_elementos_actual = 0;
        indice->cantidad_elementos_maxima = 0;
        return ERROR;
    }
    indice->cantidad_elementos_actual = 0;
    indice->cantidad_elementos_maxima = nmemb;
    return OK;
}

/******************************************************************************
Descripcion:    redimensiona el arreglo interno del indice con realloc. La
                nueva capacidad es la actual multiplicada por INCREMENTO (30%
                mas); la calcula la propia funcion.
Parametros:     indice: TDA indice.
                tamanyo: espacio en bytes ocupado por cada elemento.
Retorno:        OK si la operacion fue exitosa y ERROR en caso contrario.
Observaciones:  Si el producto no aumenta la capacidad (arreglos muy chicos)
                se garantiza al menos un elemento mas.
******************************************************************************/
int indice_redimensionar_ALU(t_indice *indice, size_t tamanyo)
{
    size_t inc = (indice->cantidad_elementos_maxima * INCREMENTO);
    void* temp = realloc(indice->vindice,tamanyo * inc);
    if(temp == NULL){
        printf("ERROR AL REDIMENSIONAR EL INDICE...");
        return ERROR;
    }
    indice->cantidad_elementos_maxima=inc;
    indice->vindice=temp;
    return OK;
}

/******************************************************************************
Descripcion:    inserta un elemento manteniendo el orden segun la clave. Si el
                arreglo esta lleno invoca indice_redimensionar.
Parametros:     indice: TDA indice.
                registro: elemento a insertar.
                tamanyo: espacio en bytes ocupado por el elemento.
                cmp: funcion de comparacion provista.
Retorno:        OK si la operacion fue exitosa y ERROR en caso contrario
                (clave duplicada o falta de memoria).
Observaciones:  -
******************************************************************************/
int indice_insertar_ALU(t_indice *indice, const void *registro, size_t tamanyo,
                    int (*cmp)(const void *, const void *))
{
    int pos=indice_buscar_ALU(indice,(void*)registro,tamanyo,cmp);
    if(pos!=-1)
        return ERROR; //ya existe, no se puede insertar
    if(indice_lleno_ALU(indice)){
        indice_redimensionar_ALU(indice, tamanyo);
    }
    char* base=(char*)indice->vindice;
    size_t i = indice->cantidad_elementos_actual;
    while(i>0&&cmp(registro,base+(i-1)*tamanyo)<0){
        memcpy(base+(i)*tamanyo,base+(i-1)*tamanyo,tamanyo);
        i--;
    }
    memcpy(base+i*tamanyo,registro,tamanyo);
    indice->cantidad_elementos_actual++;
    return OK;
}

/******************************************************************************
Descripcion:    elimina el elemento cuya clave coincide con la de registro,
                desplazando los siguientes.
Parametros:     indice: TDA indice.
                registro: elemento a eliminar (solo importa su clave).
                tamanyo: espacio en bytes ocupado por el elemento.
                cmp: funcion de comparacion provista.
Retorno:        OK si la operacion fue exitosa y ERROR si la clave no existe.
Observaciones:  -
******************************************************************************/
int indice_eliminar_ALU(t_indice *indice, const void *registro, size_t tamanyo,
                    int (*cmp)(const void *, const void *))
{
    int pos=indice_buscar_ALU(indice,(void*)registro,tamanyo,cmp);
    if(pos==-1)
        return ERROR;
    char* base=(char*)indice->vindice;
    memmove(base+pos*tamanyo,base+(pos+1)*tamanyo,(indice->cantidad_elementos_actual-pos-1)*tamanyo);
    indice->cantidad_elementos_actual--;
    return OK;
}

/******************************************************************************
Descripcion:    busca la clave de registro en el indice. Si existe copia el
                elemento completo (nro_reg incluido) en registro.
Parametros:     indice: TDA indice.
                registro: elemento a buscar (entrada/salida).
                tamanyo: espacio en bytes ocupado por el elemento.
                cmp: funcion de comparacion provista.
Retorno:        NO_EXISTE si la clave no existe; en caso contrario la posicion
                que ocupa dentro del arreglo.
Observaciones:  La comparacion entre claves se realiza exclusivamente mediante
                cmp.
******************************************************************************/
int indice_buscar_ALU(const t_indice *indice, void *registro, size_t tamanyo,
                  int (*cmp)(const void *, const void *))
{
    if(indice->cantidad_elementos_actual == 0)
        return NO_EXISTE;

    size_t low = 0, high = indice->cantidad_elementos_actual - 1, mid;
    int comp_result;

    while(low <= high){
        mid = (low + high) / 2;
        void *actual = (char *)indice->vindice + mid * tamanyo;
        comp_result = cmp(registro, actual);

        if(comp_result == 0){
            memcpy(registro, actual, tamanyo);
            return (int)mid;
        }
        else if(comp_result < 0){
            if(mid == 0) break;
            high = mid - 1;
        }else{
            low = mid + 1;
        }
    }
    return NO_EXISTE;
}

/******************************************************************************
Descripcion:    determina si el indice contiene 0 (cero) elementos.
Parametros:     indice: TDA indice.
Retorno:        OK si esta vacio, ERROR si no lo esta.
Observaciones:  -
******************************************************************************/
int indice_vacio_ALU(const t_indice *indice)
{
    if(indice->cantidad_elementos_actual==0)
        return OK;
    return ERROR;
}

/******************************************************************************
Descripcion:    determina si el indice alcanzo su capacidad maxima actual.
Parametros:     indice: TDA indice.
Retorno:        OK si esta lleno, ERROR si no lo esta.
Observaciones:  -
******************************************************************************/
int indice_lleno_ALU(const t_indice *indice)
{
    if(indice->cantidad_elementos_actual==indice->cantidad_elementos_maxima)
        return OK;
    return ERROR;
}

/******************************************************************************
Descripcion:    deja el indice vacio sin liberar la memoria del arreglo.
Parametros:     indice: TDA indice.
Retorno:        No posee.
Observaciones:  -
******************************************************************************/
void indice_vaciar_ALU(t_indice *indice)
{
    if(indice->vindice!=NULL)
        indice->vindice=NULL;

    indice->cantidad_elementos_actual=0;
    indice->cantidad_elementos_maxima=0;
}

/******************************************************************************
Descripcion:    libera toda la memoria dinamica del indice.
Parametros:     indice: TDA indice.
Retorno:        No posee.
Observaciones:  Luego de invocarla el indice no puede usarse sin volver a
                llamar a indice_crear.
******************************************************************************/
void indice_destruir_ALU(t_indice *indice)
{
    if (indice == NULL)
        return;

    free(indice->vindice);

    indice->vindice = NULL;
    indice->cantidad_elementos_actual = 0;
    indice->cantidad_elementos_maxima = 0;
}

/******************************************************************************
Descripcion:    construye el indice a partir de un archivo binario de
                registros. Recorre el archivo secuencialmente y por cada
                registro activo inserta un t_reg_indice (nro_reg = posicion en
                el archivo, id = clave obtenida con obtener_clave).
Parametros:     path: ruta al archivo binario.
                indice: TDA indice (ya creado; se vacia antes de cargar).
                tamanyo_reg: tamanyo en bytes de cada registro del archivo.
                obtener_clave: funcion que devuelve la clave (long) de un
                registro.
                es_activo: funcion que devuelve distinto de 0 si el registro
                tiene estado 'A'.
                cmp: funcion de comparacion provista.
Retorno:        OK si la operacion fue exitosa y ERROR en caso contrario.
Observaciones:  Los registros con estado 'B' se omiten. El archivo no necesita
                estar ordenado: el orden lo garantiza indice_insertar.
******************************************************************************/
int indice_cargar_ALU(const char *path, t_indice *indice, size_t tamanyo_reg,
                  long (*obtener_clave)(const void *),
                  int (*es_activo)(const void *),
                  int (*cmp)(const void *, const void *))
{
    FILE* archbin = fopen(path, "rb");
    if(!archbin)
        return ERROR;


    indice_vaciar_ALU(indice);

    void *registro = malloc(tamanyo_reg);
    if(!registro) {
        fclose(archbin);
        return ERROR;
    }

    unsigned nro_reg = 0;
    size_t leidos = fread(registro, tamanyo_reg, 1, archbin);

    while(leidos == 1){
        if(es_activo(registro)){
            t_reg_indice reg_ind;
            reg_ind.nro_reg = nro_reg;                // Posición en el archivo
            reg_ind.id = obtener_clave(registro);    // Clave obtenida mediante la función

            if(indice_insertar_ALU(indice, &reg_ind, sizeof(t_reg_indice), cmp) == ERROR) {
                free(registro);
                fclose(archbin);
                return ERROR;
            }
        }

        nro_reg++;
        leidos = fread(registro, tamanyo_reg, 1, archbin);
    }

    free(registro);
    fclose(archbin);
    return OK;
}
