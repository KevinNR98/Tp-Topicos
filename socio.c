#include <stdlib.h>
#include <string.h>
#include "socio.h"
#include "cadena.h"
#include "util.h"
#include "auditoria.h"
#include "config.h"
#include "menu.h"


static char parsear_sexo(const char *cad);
static long parsear_dni(const char *cad);
static int dni_duplicado(const t_contexto *ctx, long dni);
/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    convierte una linea CSV de 10 campos en t_socio. Los campos
                con formato invalido quedan con valores que haran fallar la
                validacion correspondiente (dni = 0, fecha 0/0/0, sexo '?',
                etc.). El estado se fija en 'A'.
Parametros:     linea: linea CSV.
                reg: puntero a t_socio destino.
Retorno:        OK si la cantidad de campos es correcta, ERROR en caso
                contrario.
Observaciones:  Se pasa por puntero como parsear a leer_archivo_texto.
******************************************************************************/
int texto_a_socio_ALU(const char *linea, void *reg)
{
    if(!linea || !reg)
        return ERROR;


    t_socio *socio = (t_socio*)reg;
    char buff[TAM_LINEA];
    char *campos[CAMPOS_SOCIO + 1];//por si la linea que leemos tiene campos de mas

    copiar_cadena(buff, linea,sizeof(buff));
    quitar_salto(buff);

    int cant = separar_campos_ALU(buff, campos, CAMPOS_SOCIO + 1);
    if(cant != CAMPOS_SOCIO)
        return ERROR;

    //dni
    socio->dni = parsear_dni(campos[0]);


    //cuil
    copiar_cadena(socio->cuil, campos[1], sizeof(socio->cuil));

    //AyN
    copiar_cadena(socio->apellidos_nombres, campos[2], sizeof(socio->apellidos_nombres));

    //fecha nac
    texto_a_fecha(campos[3], &socio->fecha_nacimiento);

    //sexo
    socio->sexo = parsear_sexo(campos[4]);

    //fecha de afiliacion
    texto_a_fecha(campos[5], &socio->fecha_afiliacion);

    //categoria
    copiar_cadena(socio->categoria, campos[6], sizeof(socio->categoria));

    //fecha de ultima cuota paga
    texto_a_fecha(campos[7], &socio->fecha_ultima_cuota);

    //plan
    copiar_cadena(socio->plan, campos[8], sizeof(socio->plan));

    //email tutor
    copiar_cadena(socio->email_tutor, campos[9], sizeof(socio->email_tutor));

    //estado inicial
    socio->estado = 'A';


    return OK;
}

static long parsear_dni(const char *cad)
{
    if(!cad || !es_numero(cad))
        return 0;

    if(*cad == '-')
        return 0;

    int cant = strlen(cad);
    if(cant < 1 || cant > 8)
        return 0;

    return atol(cad);
}

static char parsear_sexo(const char *cad)
{
    if(!cad || *cad == '\0' || *(cad + 1) != '\0')
        return '?';

    if(*cad == 'H' || *cad == 'M')
        return *cad;

    return '?';

}


/******************************************************************************
Descripcion:    aplica las validaciones del punto 6.1 en orden (DNI, Sexo,
                CUIL, Apellidos y Nombres, Fecha de nacimiento, Fecha de
                afiliacion, Categoria, Fecha de ultima cuota, Plan, Email
                tutor). Normaliza apellidos y nombres. Se detiene en el primer
                error.
Parametros:     reg: puntero a t_socio a validar (se modifica al normalizar).
                ctx: contexto con indice, archivo binario, fecha de proceso e
                id_modificando.
                tipo_error: salida; nombre de la validacion que fallo.
Retorno:        OK si el socio es valido, ERROR en caso contrario.
Observaciones:  Si ctx->id_modificando == dni no se considera duplicado y se
                excluye ese DNI al verificar unicidad del email. Cumple con el
                puntero validar de leer_archivo_texto.
******************************************************************************/
int valida_socio_ALU(void *reg, const t_contexto *ctx, char *tipo_error)
{
    if(!reg || !ctx || !tipo_error)
        return ERROR;

    t_socio *socio = (t_socio*)reg;

    //1.dni
    if(socio->dni <= DNI_MIN || socio->dni >= DNI_MAX ||dni_duplicado(ctx, socio->dni))
    {
        copiar_cadena(tipo_error, TIPO_ERR_DNI, TAM_TIPO_ERROR);
        return ERROR;
    }

    //2.sexo
    if(socio->sexo != 'H' && socio->sexo != 'M')
    {
        copiar_cadena(tipo_error, TIPO_ERR_SEXO, TAM_TIPO_ERROR);
        return ERROR;
    }


    //3.cuil
    char cuil[TAM_CUIL];
    calcular_cuil_ALU(socio->dni, socio->sexo, cuil);
    if(strcmp(socio->cuil, cuil) != 0)
    {
        copiar_cadena(tipo_error, TIPO_ERR_CUIL, TAM_TIPO_ERROR);
        return ERROR;
    }

    //4. apellido y nombre
    arreglar_apyn_ALU(socio->apellidos_nombres);
    if(*socio->apellidos_nombres == '\0')
    {
        copiar_cadena(tipo_error, TIPO_ERR_APELLIDOS_NOMBRES, TAM_TIPO_ERROR);
        return ERROR;
    }

    //5. fecha de nacimiento
    if(!fecha_correcta(&socio->fecha_nacimiento) ||
            compara_fechas(&socio->fecha_nacimiento, ctx->fecha_proceso) >= 0 ||
            edad(&socio->fecha_nacimiento, ctx->fecha_proceso) < EDAD_MINIMA)
    {
        copiar_cadena(tipo_error, TIPO_ERR_FECHA_NACIMIENTO, TAM_TIPO_ERROR);
        return ERROR;
    }

    //6. fecha de afiliacion
    if(!fecha_correcta(&socio->fecha_afiliacion) ||
            compara_fechas(&socio->fecha_afiliacion, &socio->fecha_nacimiento) <= 0 ||
            compara_fechas(&socio->fecha_afiliacion, ctx->fecha_proceso) > 0)
    {
        copiar_cadena(tipo_error, TIPO_ERR_FECHA_AFILIACION, TAM_TIPO_ERROR);
        return ERROR;
    }

    //7.categoria
    int e = edad(&socio->fecha_nacimiento, ctx->fecha_proceso);
    int es_menor = (strcmp(socio->categoria, CAT_MENOR) == 0);
    int es_adulto = (strcmp(socio->categoria, CAT_ADULTO) == 0);

    if((!es_menor && !es_adulto) ||
       (es_menor && e >= EDAD_ADULTO) ||
       (es_adulto && e < EDAD_ADULTO))
    {
        copiar_cadena(tipo_error, TIPO_ERR_CATEGORIA, TAM_TIPO_ERROR);
        return ERROR;
    }


    //8. fecha de ultima cuota paga
    if(!fecha_correcta(&socio->fecha_ultima_cuota) ||
            compara_fechas(&socio->fecha_ultima_cuota, &socio->fecha_afiliacion) < 0 ||
            compara_fechas(&socio->fecha_ultima_cuota, ctx->fecha_proceso) > 0)
    {
        copiar_cadena(tipo_error, TIPO_ERR_FECHA_ULTIMA_CUOTA, TAM_TIPO_ERROR);
        return ERROR;
    }

    //9.plan
    if(plan_indice(socio->plan) == NO_EXISTE)
    {
        copiar_cadena(tipo_error, TIPO_ERR_PLAN, TAM_TIPO_ERROR);
        return ERROR;
    }

    //10.email tutor
    if(es_adulto)
    {
        if(*(socio->email_tutor) != '\0')
        {
            copiar_cadena(tipo_error, TIPO_ERR_EMAIL_TUTOR, TAM_TIPO_ERROR);
            return ERROR;
        }
    }
    else //menor
    {
        if(*(socio->email_tutor) == '\0' ||
           !email_valido_ALU(socio->email_tutor) ||
           socio_email_existe(ctx->arch_bin, ctx->indice, socio->email_tutor, ctx->id_modificando))
        {
            copiar_cadena(tipo_error, TIPO_ERR_EMAIL_TUTOR, TAM_TIPO_ERROR);
            return ERROR;
        }
    }

    //si paso todas las validaciones
    *tipo_error = '\0';

    return OK;
}

static int dni_duplicado(const t_contexto *ctx, long dni)
{
    if(ctx->id_modificando == dni)//si se esta modificando este mismo socio no cuenta como duplicado
        return 0;

    t_reg_indice reg_ind;
    reg_ind.id = dni;

    return (indice_buscar(ctx->indice, &reg_ind, sizeof(t_reg_indice),compara_reg_indice_ALU) != NO_EXISTE);
}

/******************************************************************************
Descripcion:    lee el registro nro_reg del archivo binario de socios.
Parametros:     arch: archivo abierto en modo binario.
                nro_reg: numero de registro.
                s: socio destino.
Retorno:        OK si se pudo leer, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int socio_leer_ALU(FILE *arch, unsigned nro_reg, t_socio *s)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socio_leer(arch, nro_reg, s);
}

/******************************************************************************
Descripcion:    escribe el socio en la posicion nro_reg del archivo binario y
                hace flush.
Parametros:     arch: archivo abierto en modo de escritura binaria.
                nro_reg: numero de registro.
                s: socio a grabar.
Retorno:        OK si se pudo grabar, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int socio_escribir_ALU(FILE *arch, unsigned nro_reg, const t_socio *s)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socio_escribir(arch, nro_reg, s);
}

/******************************************************************************
Descripcion:    aplica la regla del Anexo A.4: el socio esta al dia si la
                ultima cuota es del mes de la fecha de proceso, o del mes
                anterior cuando el dia de la fecha de proceso es <= 10.
Parametros:     ultima_cuota: fecha de ultima cuota paga.
                fecha_proceso: fecha de proceso.
Retorno:        1 si esta al dia, 0 en caso contrario.
Observaciones:  -
******************************************************************************/
int cuota_al_dia_ALU(const t_fecha *ultima_cuota, const t_fecha *fecha_proceso)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return cuota_al_dia(ultima_cuota, fecha_proceso);
}

/******************************************************************************
Descripcion:    opcion Alta: carga un socio por teclado, lo valida con
                valida_socio, lo agrega al final de socios.dat con estado 'A'
                y lo inserta en el indice. Si es rechazado lo audita.
Parametros:     indice: indice de socios.
                path: ruta de socios.dat.
                fecha_proceso: fecha de proceso.
Retorno:        OK si el socio fue agregado, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int socio_alta_manual_ALU(t_indice *indice, const char *path, const t_fecha *fecha_proceso)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socio_alta_manual(indice, path, fecha_proceso);
}

/******************************************************************************
Descripcion:    opcion Modificacion: pide el DNI, lo busca en el indice (un
                socio dado de baja no se puede modificar), muestra los datos
                actuales, pide los nuevos (Enter conserva), valida y
                sobrescribe el registro. El DNI y el estado no se modifican.
                Si es rechazado lo audita.
Parametros:     indice: indice de socios.
                path: ruta de socios.dat.
                fecha_proceso: fecha de proceso.
Retorno:        OK si se modifico, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int socio_modificacion_manual_ALU(t_indice *indice, const char *path, const t_fecha *fecha_proceso)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socio_modificacion_manual(indice, path, fecha_proceso);
}

/******************************************************************************
Descripcion:    baja logica de socio: busca el DNI en el indice, pone estado
                'B' en el registro del archivo y lo elimina del indice.
Parametros:     indice: indice de socios.
                path: ruta de socios.dat.
Retorno:        OK si se dio de baja, ERROR si no existe o ya estaba dado de
                baja.
Observaciones:  -
******************************************************************************/
int socio_baja_ALU(t_indice *indice, const char *path)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socio_baja(indice, path);
}

/******************************************************************************
Descripcion:    recorre el indice (ordenado por DNI y solo con activos) y
                lista los socios leyendo cada registro del archivo.
Parametros:     indice: indice de socios.
                path: ruta de socios.dat.
Retorno:        OK si se pudo abrir el archivo, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int socios_listar_ALU(const t_indice *indice, const char *path)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socios_listar(indice, path);
}

/******************************************************************************
Descripcion:    recorre el indice y exporta a un archivo binario los socios
                con plan PREMIUM o VIP y cuota al dia (el estado 'A' lo
                garantiza el indice). Sobrescribe el archivo de exportacion
                anterior.
Parametros:     indice: indice de socios.
                path: ruta de socios.dat.
                path_export: ruta de beta_testers.dat.
                fecha_proceso: fecha de proceso.
Retorno:        OK si se pudo exportar, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int socios_exportar_ALU(const t_indice *indice, const char *path, const char *path_export,
                        const t_fecha *fecha_proceso)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return socios_exportar(indice, path, path_export, fecha_proceso);
}

/******************************************************************************
Descripcion:    opcion 2 del menu: submenu para elegir entre alta y
                modificacion manual de socio.
Parametros:     ind_socios: indice de socios.
                fecha_proceso: fecha de proceso.
Retorno:        No posee.
Observaciones:  -
******************************************************************************/
void abm_socio_ALU(t_indice *indice, const t_fecha *fecha_proceso)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    abm_socio(indice, fecha_proceso);
}
