#include <stdlib.h>
#include <string.h>
#include "titulo.h"
#include "cadena.h"
#include "util.h"


static int parsear_id(const char *cad);
static int parsear_stock(const char *cad);
static int id_duplicado(const t_contexto *ctx, int id);

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    convierte una linea CSV de 4 campos en t_titulo. Los campos
                numericos invalidos quedan en valores que fallan la validacion
                (id = 0, stock = -1). El estado se fija en 'A'.
Parametros:     linea: linea CSV.
                reg: puntero a t_titulo destino.
Retorno:        OK si la cantidad de campos es correcta, ERROR en caso
                contrario.
Observaciones:  Se pasa por puntero como parsear a leer_archivo_texto.
******************************************************************************/
int texto_a_titulo_ALU(const char *linea, void *reg)
{
    if(!linea || !reg)
        return ERROR;


    t_titulo *titulo = (t_titulo*)reg;
    char buff[TAM_LINEA];
    char *campos[CAMPOS_TITULO + 1];

    copiar_cadena(buff, linea, sizeof(buff));
    quitar_salto(buff);

    int cant = separar_campos_ALU(buff, campos, CAMPOS_TITULO + 1);
    if(cant != CAMPOS_TITULO)
        return ERROR;

    //1.id pelicula
    titulo->id = parsear_id(campos[0]);

    //2.titulo
    copiar_cadena(titulo->titulo,campos[1], sizeof(titulo->titulo));

    //3.genero
    copiar_cadena(titulo->genero, campos[2], sizeof(titulo->genero));

    //4.stock
    titulo->stock = parsear_stock(campos[3]);

    //estado inicial
    titulo->estado = 'A';

    return OK;
}


static int parsear_id(const char *cad)
{
    if (!cad || !es_numero(cad) || *cad == '-')
        return 0;

    int len = strlen(cad);
    if (len < 1 || len > 9)
        return 0;

    long val = atol(cad);

    return (int)val;
}

static int parsear_stock(const char *cad)
{
    if (!cad || !es_numero(cad))
        return -1;

    //si es negativo o excede digitos
    if (*cad == '-' || strlen(cad) > 9)
        return -1;

    long val = atol(cad);

    return (int)val;
}

/******************************************************************************
Descripcion:    aplica las validaciones del punto 6.2 en orden: ID > 0 y no
                existente en el indice, titulo normalizado no vacio, genero
                dentro de GENEROS[] y stock >= 0. Se detiene en el primer
                error.
Parametros:     reg: puntero a t_titulo (se modifica al normalizar el titulo).
                ctx: contexto con indice e id_modificando.
                tipo_error: salida; nombre de la validacion que fallo.
Retorno:        OK si el titulo es valido, ERROR en caso contrario.
Observaciones:  Se pasa por puntero como validar a leer_archivo_texto. Se reutiliza en las altas del
                lote de novedades.
******************************************************************************/
int valida_titulo_ALU(void *reg, const t_contexto *ctx, char *tipo_error)
{
    if(!reg || !ctx || !tipo_error)
        return ERROR;

    t_titulo *titulo = (t_titulo*)reg;

    //1.id
    if(titulo->id <= 0 || id_duplicado(ctx, titulo->id))
    {
        copiar_cadena(tipo_error, TIPO_ERR_ID, TAM_TIPO_ERROR);
        return ERROR;
    }


    //2.titulo
    arreglar_titulo(titulo->titulo);
    if(*titulo->titulo == '\0')
    {
        copiar_cadena(tipo_error, TIPO_ERR_TITULO, TAM_TIPO_ERROR);
        return ERROR;
    }

    //3.genero
    if(genero_indice(titulo->genero) < 0)
    {
        copiar_cadena(tipo_error, TIPO_ERR_GENERO, TAM_TIPO_ERROR);
        return ERROR;
    }

    //4.stock
    if(titulo->stock < 0)
    {
        copiar_cadena(tipo_error, TIPO_ERR_STOCK, TAM_TIPO_ERROR);
        return ERROR;
    }

    *tipo_error = '\0';
    return OK;
}


static int id_duplicado(const t_contexto *ctx, int id)
{
    if(ctx->id_modificando == id)
        return 0;

    t_reg_indice reg_ind;
    reg_ind.id = id;

    return indice_buscar_ALU(ctx->indice,
                             &reg_ind,
                             sizeof(t_reg_indice),
                             compara_reg_indice_ALU) != NO_EXISTE;
}


/******************************************************************************
Descripcion:    recorre el indice (ordenado por ID y solo con activos) y lista
                los titulos leyendo cada registro del archivo.
Parametros:     indice: indice de titulos.
                path: ruta de titulos.dat.
Retorno:        OK si se pudo abrir el archivo, ERROR en caso contrario.
Observaciones:  -
******************************************************************************/
int titulos_listar_ALU(const t_indice *indice, const char *path)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return titulos_listar(indice, path);
}
