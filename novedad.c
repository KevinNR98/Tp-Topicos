#include <string.h>
#include "novedad.h"
#include "cadena.h"

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

/******************************************************************************
Descripcion:    recorre secuencialmente novedades_catalogo.dat y aplica cada
                operacion sobre el indice y titulos.dat: 'A' agrega si el ID
                no esta en el indice (validando como en la carga), 'B' pone
                estado 'B' en el archivo y elimina del indice, 'M' reemplaza
                titulo y genero y suma el stock (rechaza si quedaria
                negativo). Al final muestra el reporte de procesadas y
                rechazadas.
Parametros:     indice: indice de titulos.
                path_novedades: ruta de novedades_catalogo.dat.
                path_titulos: ruta de titulos.dat.
                fecha_proceso: fecha de proceso (para el contexto de
                validacion).
Retorno:        OK si se pudieron abrir los archivos, ERROR en caso contrario.
Observaciones:  Como el indice solo tiene activos, un 'A' sobre un ID dado de
                baja se agrega como registro nuevo al final del archivo.
******************************************************************************/
int novedades_procesar_ALU(t_indice *indice, const char *path_novedades,
                       const char *path_titulos, const t_fecha *fecha_proceso)
{
    FILE *f_nov = fopen(path_novedades, "rb");
    FILE *f_tit = fopen(path_titulos, "r+b");

    if(!f_nov || !f_tit)
        return ERROR;

    t_novedad novedad;
    t_reg_indice reg_buscado;
    t_titulo titulo;
    t_rechazo rechazos[MAX_RECHAZOS] = {0};
    int c_exitos = 0, c_rechazos = 0;

    t_rechazo *p_rec = rechazos;

    fread(&novedad, sizeof(t_novedad), 1, f_nov);

    while(!feof(f_nov))
    {
        reg_buscado.id = novedad.titulo.id;
        switch(novedad.codigo_operacion)
        {
         case 'A':
            if(indice_buscar_ALU(indice, &reg_buscado, sizeof(t_reg_indice), compara_reg_indice_ALU) == NO_EXISTE)
            {
                t_contexto ctx = {indice, f_tit, fecha_proceso, 0};
                if(valida_titulo_ALU(&novedad.titulo, &ctx, p_rec->motivo) != OK)
                {
                    p_rec->id = novedad.titulo.id;
                    c_rechazos++;
                    p_rec++;
                    break;
                }
                novedad.titulo.estado = 'A';
                fseek(f_tit, 0, SEEK_END);
                reg_buscado.nro_reg = (unsigned)(ftell(f_tit) / sizeof(t_titulo));
                fwrite(&novedad.titulo, sizeof(t_titulo), 1, f_tit);
                fflush(f_tit);
                indice_insertar(indice, &reg_buscado, sizeof(t_reg_indice), compara_reg_indice_ALU);
                c_exitos++;
            }
            else
            {
                p_rec->id = novedad.titulo.id;
                strcpy(p_rec->motivo, "ID DUPLICADO");
                c_rechazos++;
                p_rec++;
            }
            break;
        case 'B':
            if(indice_buscar_ALU(indice, &reg_buscado, sizeof(t_reg_indice), compara_reg_indice_ALU) != NO_EXISTE)
            {
                fseek(f_tit, sizeof(t_titulo) * reg_buscado.nro_reg, SEEK_SET);
                fread(&titulo, sizeof(t_titulo), 1, f_tit);
                titulo.estado = 'B';
                fseek(f_tit, sizeof(t_titulo) * reg_buscado.nro_reg, SEEK_SET);
                fwrite(&titulo, sizeof(t_titulo), 1, f_tit);
                fflush(f_tit);
                indice_eliminar_ALU(indice, &reg_buscado, sizeof(t_reg_indice), compara_reg_indice_ALU);
                c_exitos++;
            }
            else
            {
                p_rec->id = novedad.titulo.id;
                strcpy(p_rec->motivo, "ID NO ENCONTRADO");
                c_rechazos++;
                p_rec++;
            }
            break;
        case 'M':
            if(indice_buscar_ALU(indice, &reg_buscado, sizeof(t_reg_indice), compara_reg_indice_ALU) != NO_EXISTE)
            {
                t_contexto ctx = {indice, f_tit, fecha_proceso, reg_buscado.id};

                fseek(f_tit, sizeof(t_titulo) * reg_buscado.nro_reg, SEEK_SET);
                fread(&titulo, sizeof(t_titulo), 1, f_tit);

                novedad.titulo.stock += titulo.stock;

                if(valida_titulo_ALU(&novedad.titulo, &ctx, p_rec->motivo) != OK)
                {
                    p_rec->id = novedad.titulo.id;
                    c_rechazos++;
                    p_rec++;
                    break;
                }

                strcpy(titulo.titulo, novedad.titulo.titulo);
                strcpy(titulo.genero, novedad.titulo.genero);
                titulo.stock = novedad.titulo.stock;

                fseek(f_tit, sizeof(t_titulo) * reg_buscado.nro_reg, SEEK_SET);
                fwrite(&titulo, sizeof(t_titulo), 1, f_tit);
                fflush(f_tit);
                c_exitos++;
            }
            else
            {
                p_rec->id = novedad.titulo.id;
                strcpy(p_rec->motivo, "ID NO ENCONTRADO");
                c_rechazos++;
                p_rec++;
            }
            break;
        default:
                p_rec->id = novedad.titulo.id;
                strcpy(p_rec->motivo, "CODIGO OPERACION INVALIDO");
                c_rechazos++;
                p_rec++;
                break;
        }
        fread(&novedad, sizeof(t_novedad), 1, f_nov);
    }

    fclose(f_tit);
    fclose(f_nov);

    printf("REPORTE DE NOVEDADES PROCESADAS\n\nCantidad de exitos: %d\nCantidad de rechazos: %d\n\n", c_exitos, c_rechazos);

    if(c_rechazos > 0)
    {
        p_rec = rechazos;
        puts("RECHAZOS\n");
        for(int i=0; i < c_rechazos; i++, p_rec++)
            printf("ID: %d\t\tMOTIVO: %s\n\n", p_rec->id, p_rec->motivo);
    }

    return OK;
}
