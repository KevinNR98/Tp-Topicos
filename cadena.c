#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "cadena.h"

#define MAX_STR 61

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    normaliza un campo Apellidos y Nombres segun el Anexo A.2 (con
                regla de coma).
Parametros:     s: cadena a normalizar (se modifica in place).
Retorno:        No posee.
Observaciones:  -
******************************************************************************/
void arreglar_apyn_ALU(char *s)
{
    if (!s)
        return;

    char temp[MAX_STR] = {0};
    char *p_src = s;
    char *p_dst = temp;
    int coma_puesta = 0;
    int nueva_palabra = 1;

    while (*p_src != '\0' && (p_dst - temp) < MAX_STR - 2)
    {
        if (*p_src == ',' || isspace((unsigned char)*p_src))
        {
            //solo actua si no esta al inicio y el ultimo caracter escrito no es un espacio
            if (p_dst > temp && *(p_dst - 1) != ' ')
            {
                //si aun no se fijo el apellido o si es un espacio entre nombres
                if (!coma_puesta || isspace((unsigned char)*p_src))
                {
                    if (!coma_puesta)
                    {
                        *p_dst++ = ',';
                        coma_puesta = 1;
                    }
                    *p_dst++ = ' ';
                    nueva_palabra = 1;
                }
                //si coma_puesta == 1 y vino otra coma se ignora
            }
        }
        else //letras del apellido o nombre
        {
            if (nueva_palabra)
            {
                *p_dst++ = toupper((unsigned char)*p_src);
                nueva_palabra = 0;
            }
            else
            {
                *p_dst++ = tolower((unsigned char)*p_src);
            }
        }
        p_src++;
    }

    //limpieza de delimitadores colgantes al final
    if (p_dst > temp && *(p_dst - 1) == ' ') p_dst--;
    if (p_dst > temp && *(p_dst - 1) == ',') p_dst--;
    *p_dst = '\0';

    //copia el resultado formateado de regreso a s
    p_src = s;
    p_dst = temp;
    while (*p_dst != '\0')
    {
        *p_src++ = *p_dst++;
    }
    *p_src = '\0';
}

/******************************************************************************
Descripcion:    separa una linea CSV in place por SEPARADOR_CSV, reemplazando
                cada separador por '\0' y guardando en campos[] el puntero al
                inicio de cada campo.
Parametros:     linea: linea a separar (se modifica).
                campos: vector donde se guardan los punteros a cada campo.
                max: cantidad maxima de campos a separar.
Retorno:        Cantidad de campos encontrados (los vacios cuentan). Nunca mas
                de max.
Observaciones:  Si la linea tiene mas separadores que max, el ultimo campo
                conserva el resto de la linea.
******************************************************************************/
int separar_campos_ALU(char *linea, char *campos[], int max)
{
    if(!linea || !campos || max <= 0)
        return 0;

    //si la linea esta vacia no contiene campos
    if(*linea == '\0')
        return 0;

    int cant = 0;
    campos[cant++] = linea;//el primer campo siempre inicia al comienzo de la linea

    while(*linea != '\0' && cant < max)//recorre la cadena mientras haya caracteres y no se haya alcanzado todos los campos
    {
        if(*linea == SEPARADOR_CSV)
        {
            *linea = '\0';              //corta en el campo actual
            campos[cant++] = linea + 1; //el siguiente campo arranca tras el separador
        }
        linea++;
    }
    //al detener la busqueda cuando cant == max; cualquier separador restante no se reemplaza y queda formando parte del ultimo campo
    return cant;
}
