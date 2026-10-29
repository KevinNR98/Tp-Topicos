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
    if (!pf)
        return ERROR;

    *aceptados = 0;
    *rechazados = 0;

    char linea[TAM_LINEA + 1];
    char error_msg[TAM_TIPO_ERROR];

    // Armar el contexto que se compartirá en cada validación
    t_contexto ctx;
    ctx.indice = indice;
    ctx.arch_bin = arch_bin;
    ctx.fecha_proceso = fecha_proceso;
    ctx.id_modificando = 0; // 0 porque son altas masivas desde el archivo de texto

    // Recorrer el archivo de texto línea por línea
    while (fgets(linea, sizeof(linea), pf))
    {

        // Parsear la línea al buffer de registro
        if (parsear(linea, reg) != OK) //
        {
            rechazar(reg, linea, "FORMATO", fecha_proceso);
            (*rechazados)++;
            continue;
        }

        // Validar el registro pasándole el contexto
        if (validar(reg, &ctx, error_msg) != OK)
        {
            rechazar(reg, linea, error_msg, fecha_proceso);
            (*rechazados)++;
            continue;
        }

        fseek(arch_bin, 0L, SEEK_END);

        long pos = ftell(arch_bin);
        unsigned nro_reg = pos / tamanyo;

        fwrite(reg, tamanyo, 1, arch_bin);
        fflush(arch_bin);

        // Insertar en el índice en memoria
        t_reg_indice reg_ind;
        reg_ind.nro_reg = nro_reg;
        reg_ind.id = obtener_clave(reg);

        if (indice_insertar_ALU(indice, &reg_ind, sizeof(t_reg_indice),
                                 compara_reg_indice_ALU) != OK)
        {
            rechazar(reg, linea, "DNI", fecha_proceso);
            (*rechazados)++;
            continue;
        }

        (*aceptados)++;
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
    const t_reg_indice *reg_a = (const t_reg_indice *)a;
    const t_reg_indice *reg_b = (const t_reg_indice *)b;

    if (reg_a->id < reg_b->id)
        return -1;
    if (reg_a->id > reg_b->id)
        return 1;

    return 0;
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
    indice_cargar_ALU(ARCH_SOCIOS_BIN, ind_socios, sizeof(t_socio), obtener_clave_socio, es_socio_activo, compara_reg_indice_ALU);
    indice_cargar_ALU(ARCH_TITULOS_BIN, ind_titulos, sizeof(t_titulo), obtener_clave_titulo, es_titulo_activo, compara_reg_indice_ALU);

    if(indice_vacio_ALU(ind_socios) || indice_vacio_ALU(ind_titulos))
        return ERROR;

    return OK;
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
        if(arch_socios) fclose(arch_socios);
        if(arch_titulos) fclose(arch_titulos);

        puts("Error en la apertura de los archivos.");
        return ERROR;
    }

    indice_vaciar_ALU(ind_socios);
    indice_vaciar_ALU(ind_titulos);

    unsigned aceptados=0, rechazados=0;
    t_socio socio;
    t_titulo titulo;

    leer_archivo_texto_ALU(ARCH_SOCIOS_CSV, arch_socios, ind_socios, &socio, sizeof(t_socio), fecha_proceso,
                           texto_a_socio_ALU, valida_socio_ALU, rechazar_socio, obtener_clave_socio, &aceptados, &rechazados);
    printf("Socios: %u aceptados %u rechazados ", aceptados, rechazados);
    if(rechazados > 0)
        printf("(ver archivos/errores_socios.txt)");
    putchar('\n');
    leer_archivo_texto_ALU(ARCH_TITULOS_CSV, arch_titulos, ind_titulos, &titulo, sizeof(t_titulo), fecha_proceso,
                           texto_a_titulo_ALU, valida_titulo_ALU, rechazar_titulo, obtener_clave_titulo, &aceptados, &rechazados);
    printf("Titulos: %u aceptados %u rechazados\n\n", aceptados, rechazados);

    fclose(arch_socios);
    fclose(arch_titulos);

    indice_destruir_ALU(ind_socios);
    indice_destruir_ALU(ind_titulos);

    if(indice_crear_ALU(ind_socios, CANTIDAD_ELEMENTOS, sizeof(t_reg_indice)) != OK ||
       indice_crear_ALU(ind_titulos, CANTIDAD_ELEMENTOS, sizeof(t_reg_indice)) != OK)
        return ERROR;

    if(cargar_indices_ALU(ind_socios, ind_titulos) != OK)
        return ERROR;

    return OK;
}
