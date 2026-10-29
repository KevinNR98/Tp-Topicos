#ifndef AUDITORIA_H_INCLUDED
#define AUDITORIA_H_INCLUDED

#include "fecha.h"
#include "socio.h"

int auditoria_registrar(const char *path, const char *dni_texto,
                        const char *tipo_error, const t_fecha *fecha_proceso);
int auditoria_mostrar(const char *path);

/*============================================================================*/
/*            FUNCIONES A IMPLEMENTAR POR EL ALUMNO (version _ALU)             */
/*============================================================================*/

int auditoria_registrar_ALU(const char *path, const char *dni_texto,
                        const char *tipo_error, const t_fecha *fecha_proceso);
int auditoria_mostrar_ALU(const char *path);

typedef struct {
    char dni[TAM_DNI];
    char tipo_error[TAM_TIPO_ERROR];
    int cantidad;
} t_auditoria;

#endif /* AUDITORIA_H_INCLUDED */
