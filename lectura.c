#include <string.h>
#include "lectura.h"
#include "util.h"
#include "cadena.h"
#include "socio.h"
#include "titulo.h"
#include "config.h"

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    funcion generica de lectura de archivos de texto. Por cada
                linea no vacia: parsear -> validar -> escribir al final del
                archivo binario e insertar en el indice. Ante el primer error
                (de formato o de validacion) se rechaza la linea invocando
                rechazar y se continua con la siguiente.
Parametros:     path_txt: ruta del archivo de texto (CSV).
                arch_bin: archivo binario destino abierto en "w+b" o "r+b".
                indice: indice de la entidad (se usa para validar duplicados y
                se va completando).
                reg: buffer de un registro de la entidad.
                tamanyo: tamanyo en bytes del registro.
                fecha_proceso: fecha de proceso.
                parsear: convierte una linea en registro.
                validar: valida el registro y deja el tipo de error.
                rechazar: accion ante un rechazo (auditar, ignorar, etc.).
                obtener_clave: devuelve la clave del registro para el indice.
                aceptados, rechazados: salida; contadores de registros.
Retorno:        OK si se pudo abrir el archivo de texto, ERROR en caso
                contrario.
Observaciones:  Se reutiliza para Socios.csv y Titulos.csv; lo especifico de
                cada entidad lo resuelven los punteros a funcion.
******************************************************************************/
int leer_archivo_texto_ALU(const char *path_txt, FILE *arch_bin, t_indice *indice,
                       void *reg, size_t tamanyo, const t_fecha *fecha_proceso,
                       int (*parsear)(const char *, void *),
                       int (*validar)(void *, const t_contexto *, char *),
                       void (*rechazar)(const void *, const char *, const char *, const t_fecha *),
                       long (*obtener_clave)(const void *),
                       unsigned *aceptados, unsigned *rechazados)
{
    FILE *pf = fopen(path_txt, "rt");

    if(!pf)
        return ERROR;

    //Linea auxiliar para registro invalido
    char linea[TAM_LINEA], tipo_error[TAM_TIPO_ERROR];
    t_contexto ctx = {indice, arch_bin, fecha_proceso, 0};
    long pos;
    t_reg_indice reg_indice;

    while(fgets(linea, TAM_LINEA, pf))
    {
        memset(reg, 0, tamanyo);
        tipo_error[0] = '\0';
        parsear(linea, reg);

        if(validar(reg, &ctx, tipo_error) == ERROR)
        {
            rechazar(reg, linea, tipo_error, fecha_proceso);
            (*rechazados)++;
        }
        else
        {
            fseek(arch_bin, 0, SEEK_END);
            pos = ftell(arch_bin);
            reg_indice.nro_reg = pos/tamanyo;
            reg_indice.id = obtener_clave(reg);

            fwrite(reg, tamanyo, 1, arch_bin);
            fflush(arch_bin);
            indice_insertar_ALU(indice, &reg_indice, sizeof(t_reg_indice), compara_reg_indice_ALU);

            (*aceptados)++;
        }
    }

    fclose(pf);
    return OK;
}

/******************************************************************************
Descripcion:    compara dos t_reg_indice por su clave id.
Parametros:     a, b: punteros a t_reg_indice.
Retorno:        Negativo si a < b, 0 si son iguales, positivo si a > b.
Observaciones:  Se usa como cmp en todas las operaciones del TDA indice.
******************************************************************************/
int compara_reg_indice_ALU(const void *a, const void *b)
{
    const t_reg_indice *reg1 = a;
    const t_reg_indice *reg2 = b;

    return reg1->id - reg2->id;
}

/******************************************************************************
Descripcion:    construye los indices de socios y de titulos a partir de los
                archivos binarios existentes (solo registros activos).
Parametros:     ind_socios: indice de socios.
                ind_titulos: indice de titulos.
Retorno:        1 si ambos binarios existen y se cargaron, 0 en caso
                contrario.
Observaciones:  -
******************************************************************************/
int cargar_indices_ALU(t_indice *ind_socios, t_indice *ind_titulos)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return cargar_indices(ind_socios, ind_titulos);
}

/******************************************************************************
Descripcion:    opcion 1 del menu: lee Socios.csv y Titulos.csv con la funcion
                generica de lectura, genera socios.dat y titulos.dat
                (sobrescribiendolos) y reconstruye los indices desde los
                binarios generados.
Parametros:     ind_socios: indice de socios.
                ind_titulos: indice de titulos.
                fecha_proceso: fecha de proceso.
Retorno:        1 si los indices quedaron cargados, 0 en caso contrario.
Observaciones:  Los rechazos de socios se auditan en errores_socios.txt; los
                de titulos solo se cuentan.
******************************************************************************/
int generar_binarios_ALU(t_indice *ind_socios, t_indice *ind_titulos, const t_fecha *fecha_proceso)
{
    FILE *arch_socios = fopen(ARCH_SOCIOS_BIN, "wb+");
    FILE *arch_titulos = fopen(ARCH_TITULOS_BIN, "wb+");

    if(!arch_socios || !arch_titulos)
    {
        puts("Error en la apertura de los archivos.");
        return ERROR;
    }

    indice_vaciar_ALU(ind_socios);
    indice_vaciar_ALU(ind_titulos);

    unsigned aceptados=0, rechazados=0;
    t_socio socio;
    t_titulo titulo;

    leer_archivo_texto_ALU(ARCH_SOCIOS_CSV, arch_socios, ind_socios, &socio, sizeof(t_socio), fecha_proceso, texto_a_socio_ALU, valida_socio_ALU, rechazar_socio, obtener_clave_socio, &aceptados, &rechazados);
    printf("Socios: %u aceptados %u rechazados ", aceptados, rechazados);
    if(rechazados > 0)
        printf("(ver archivos/errores_socios.txt)");
    putchar('\n');
    aceptados=0;
    rechazados=0;
    leer_archivo_texto_ALU(ARCH_TITULOS_CSV, arch_titulos, ind_titulos, &titulo, sizeof(t_titulo), fecha_proceso, texto_a_titulo_ALU, valida_titulo_ALU, rechazar_titulo, obtener_clave_titulo, &aceptados, &rechazados);
    printf("Titulos: %u aceptados %u rechazados\n\n", aceptados, rechazados);

    fclose(arch_socios);
    fclose(arch_titulos);

    indice_destruir_ALU(ind_socios);
    indice_destruir_ALU(ind_titulos);

    if(indice_crear_ALU(ind_socios, CANTIDAD_ELEMENTOS, sizeof(t_reg_indice)) != OK)
        return ERROR;

    if(indice_crear_ALU(ind_titulos, CANTIDAD_ELEMENTOS, sizeof(t_reg_indice)) != OK)
        return ERROR;

    indice_cargar_ALU(ARCH_SOCIOS_BIN, ind_socios, sizeof(t_socio), obtener_clave_socio, es_activo_socio, compara_reg_indice_ALU);
    indice_cargar_ALU(ARCH_TITULOS_BIN, ind_titulos, sizeof(t_titulo), obtener_clave_titulo, es_activo_titulo, compara_reg_indice_ALU);

    return OK;
}
