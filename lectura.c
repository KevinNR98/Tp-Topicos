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
    char error_msg[256];

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
            rechazar(reg, linea, "Error de formato", fecha_proceso);
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

        // Insertar en el índice en memoria
        t_reg_indice reg_ind;
        reg_ind.nro_reg = nro_reg;
        reg_ind.id = obtener_clave(reg);

        if (indice_insertar(indice, &reg_ind, sizeof(t_reg_indice), compara_reg_indice_ALU) != OK)
        {
            rechazar(reg, "Clave duplicada en indice", "Duplicado", fecha_proceso);
            (*rechazados)++;
            continue;
        }

        (*aceptados)++;
    }

    fclose(pf);
    return OK;

    //return leer_archivo_texto(path_txt, arch_bin, indice, reg, tamanyo, fecha_proceso, parsear, validar, rechazar, obtener_clave, aceptados, rechazados);
}

/******************************************************************************
Descripcion:    compara dos t_reg_indice por su clave id.
Parametros:     a, b: punteros a t_reg_indice.
Retorno:        Negativo si a < b, 0 si son iguales, positivo si a > b.
Observaciones:  Se usa como cmp en todas las operaciones del TDA indice.
******************************************************************************/
int compara_reg_indice_ALU(const void *a, const void *b)
{
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return compara_reg_indice(a, b);
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
    /* TODO: reemplazar esta llamada a la funcion de la catedra por su propia
       implementacion. En la entrega no puede quedar ninguna llamada a una
       funcion de la biblioteca que tenga version _ALU. */
    return generar_binarios(ind_socios, ind_titulos, fecha_proceso);
}
////////////////////////////////////////////////////////////////////

int parsear_titulo(const char *linea, void *reg)
{
    t_titulo *titulo = (t_titulo *)reg;

    int leidos = sscanf(linea, "%d;%60[^;];%20[^;];%d", &titulo->id, titulo->titulo, titulo->genero, &titulo->stock);

    if (leidos != 4)
        return ERROR;

    titulo->estado = 'A';

    return OK;
}

int parsear_socio(const char *linea, void *reg)
{
    t_socio *socio = (t_socio *)reg;

    // Inicializamos el email por si viene vacío en la línea
    socio->email_tutor[0] = '\0';

    // sscanf intentará leer los 16 elementos (contando día/mes/año por separado)
    int leidos = sscanf(linea, "%ld;%13[^;];%60[^;];%d/%d/%d;%c;%d/%d/%d;%10[^;];%d/%d/%d;%10[^;];%30[^\r\n]",
                        &socio->dni,
                        socio->cuil,
                        socio->apellidos_nombres,
                        &socio->fecha_nacimiento.dia,
                        &socio->fecha_nacimiento.mes,
                        &socio->fecha_nacimiento.anio,
                        &socio->sexo,
                        &socio->fecha_afiliacion.dia,
                        &socio->fecha_afiliacion.mes,
                        &socio->fecha_afiliacion.anio,
                        socio->categoria,
                        &socio->fecha_ultima_cuota.dia,
                        &socio->fecha_ultima_cuota.mes,
                        &socio->fecha_ultima_cuota.anio,
                        socio->plan,
                        socio->email_tutor);

    // Como el email_tutor es opcional al final, sscanf puede retornar 15 (si no viene el email)
    // o 16 (si viene completo). Por eso exigimos que al menos lea los primeros 15 campos obligatorios.
    if (leidos < 15)
        return ERROR;


    socio->estado = 'A';

    return OK;
}

void ignorar_rechazo_titulo(const void *reg, const char *tipo_error, const char *detalle, const t_fecha *fecha_proceso)
{
    // Ignoramos los titulos rechazados
    (void)reg;
    (void)tipo_error;
    (void)detalle;
    (void)fecha_proceso;
}

long obtener_clave_socio(const void *reg)
{
    const t_socio *socio = (const t_socio *)reg;
    return socio->dni;
}

long obtener_clave_titulo(const void *reg)
{
    const t_titulo *titulo = (const t_titulo *)reg;
    return (long)titulo->id;
}
