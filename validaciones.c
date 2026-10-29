#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "validaciones.h"

static int local_valida(const char *local, int tam);
static int dominio_valida(const char *dominio);

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    calcula el CUIL con el algoritmo modulo 11 del Anexo A.1:
                prefijo 20 (hombre) o 27 (mujer), DNI a 8 digitos y digito
                verificador. Si el resto es 1 el prefijo pasa a 23 y Z es 9
                (hombre) o 4 (mujer).
Parametros:     dni: numero de documento.
                sexo: 'H' o 'M'.
                dest: buffer de al menos TAM_CUIL bytes donde se deja "XY-
                NNNNNNNN-Z".
Retorno:        No posee.
Observaciones:  -
******************************************************************************/
void calcular_cuil_ALU(long dni, char sexo, char *dest)
{
    char codID[11];// prefijo y dni XYNNNNNNNN
    const int mult[] = {5, 4, 3, 2, 7, 6, 5, 4, 3, 2};
    const char *pc = codID;
    const int *pm = mult;

    int prefijo = 0;
    int verificador = 0;
    int suma = 0, resto;



    //si mas adelante el resto da 1 el prefijo pasa a ser PREFIJO_RESTO1 (23)
    // el digito verificador queda fijado en 9 (hombre) o 4 (mujer).
    if(sexo == 'H')
    {
        prefijo = PREFIJO_HOMBRE;
        verificador = 9; //verificadores del caso resto 1
    }
    else if(sexo == 'M')
    {
        prefijo = PREFIJO_MUJER;
        verificador = 4;//verificadores del caso resto 1
    }


    sprintf(codID, "%02d%08ld", prefijo, dni);


    while(*pc)
    {
        suma += (*pc - '0') * (*pm);//multiplica cada digito por su peso (*pm)
        pc++;
        pm++;
    }

    resto = suma % 11;

    if(resto == 0)
    {
        verificador = 0;
    }
    else if(resto == 1)
    {
        prefijo = PREFIJO_RESTO1;
    }
    else
        verificador = 11 - resto;


    sprintf(dest, "%02d-%08ld-%d", prefijo, dni, verificador);
}

/******************************************************************************
Descripcion:    valida un email segun el Anexo A.3: hasta 30 caracteres sin
                espacios, una unica '@', parte local de letras/digitos/puntos
                que no empieza ni termina con punto, y dominio de
                letras/digitos/guiones/puntos con al menos un punto, que no
                empieza ni termina con punto o guion y cuyo ultimo segmento
                tiene al menos 2 letras.
Parametros:     email: cadena a validar.
Retorno:        1 si es valido, 0 en caso contrario.
Observaciones:  No verifica unicidad; eso lo hace socio_email_existe.
******************************************************************************/
int email_valido_ALU(const char *email)
{
    int tamL = 0, tamE = 0, rt = 0;

    //control de puntero nulo
    if(!email)
        return 0;

    //validacion de longitud maxima
    tamE = strlen(email);
    if(tamE > EMAIL_MAX_LEN)
        return 0;

    //busqueda y particion por el caracter separador '@'
    const char *arroba = strchr(email, '@');

    //debe contener obligatoriamente una'@'
    if(arroba == NULL)
        return 0;

    //longitud de la parte local
    tamL = arroba - email;

    //la parte local debe tener al menos 1 caracter
    if(tamL < 1)
        return 0;

    //Validacion de la parte local, a la izquierda de '@'
    rt = local_valida(email, tamL);
    if(rt == 0)
        return 0;

    //validacion del dominio, a la derecha de '@'
    rt = dominio_valida(arroba + 1);
    if(rt == 0)
        return 0;

    return 1;
}


static int local_valida(const char *local, int tam)
{
    const char* fin = local + (tam -1);//puntero al ultimo carácter

    if(*local == '.' || *fin == '.')//no puede empezar ni terminar con punto
        return 0;

    for(int i = 0; i < tam; i++)//recorre los caracteres que conforman la parte local
    {
        ////solo permite letras/digitos o puntos, rechaza espacios y caracteres especiales
        if(isalnum((unsigned char)*local) == 0 && *local != '.')
            return 0;

        local++;
    }

    return 1;
}

static int dominio_valida(const char *dominio)
{
    const char* pPunto;
    int tam = strlen(dominio);

    if(tam < 1)//el dominio no puede ser vacio
        return 0;

    if(*dominio == '.' || *dominio == '-')//no puede empezar con punto ni con guion
        return 0;

    const char* fin = dominio + (tam - 1);//puntero al ultimo caracter del dominio
    if(*fin == '.' || *fin == '-')//no puede terminar con punto ni con guion
        return 0;

    pPunto = strrchr(dominio, '.');//busqueda del ultimo punto
    if(pPunto == NULL)             //ej: en "mail.google.com.ar", pPunto apunta a ".ar"
        return 0;

    //valida el dominio desde el inicio hasta el ultimo punto
    while(*dominio && dominio != pPunto)
    {
        if(isalnum((unsigned char)*dominio) == 0 && *dominio != '.' && *dominio != '-')
            return 0;
        dominio++;
    }
    pPunto++;//salteo el punto y quedo sobre las letras finales

    int cantLetras = 0;
    while(*pPunto)//valida el ultimo segmento
    {
        if(isalpha((unsigned char)*pPunto) == 0)
            return 0;
        cantLetras++;
        pPunto++;
    }

    if(cantLetras < 2)//el ultimo segmento debe tener al menos 2 letras
        return 0;

    return 1;
}

int validar_dni(const long dni)
{
    return (dni > 1000000 && dni < 100000000);
}
