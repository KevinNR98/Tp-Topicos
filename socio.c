#include <stdlib.h>
#include <string.h>
#include <conio.h>
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
    char *campos[CAMPOS_SOCIO + 1]; //por si la linea que leemos tiene campos de mas

    copiar_cadena(buff, linea,sizeof(buff));
    quitar_salto(buff);

    int cant = separar_campos_ALU(buff, campos, CAMPOS_SOCIO + 1);
    if(cant != CAMPOS_SOCIO)
        return ERROR;

    //1.dni
    socio->dni = parsear_dni(campos[0]);

    //2.cuil
    if(strlen(campos[1]) >= sizeof(socio->cuil))
        strcpy(socio->cuil, "00-00000000-0");
    else
        copiar_cadena(socio->cuil, campos[1], sizeof(socio->cuil));

    //3.AyN
    if(strlen(campos[2]) >= sizeof(socio->apellidos_nombres))
        socio->apellidos_nombres[0] = '\0';
    else
        copiar_cadena(socio->apellidos_nombres, campos[2], sizeof(socio->apellidos_nombres));

    //4.fecha nac
    if(!texto_a_fecha(campos[3], &socio->fecha_nacimiento))
        socio->fecha_nacimiento = (t_fecha){0, 0, 0};

    //5.sexo
    socio->sexo = parsear_sexo(campos[4]);

    //6.fecha de afiliacion
    if(!texto_a_fecha(campos[5], &socio->fecha_afiliacion))
        socio->fecha_afiliacion = (t_fecha){0, 0, 0};

    //7.categoria
    if(strlen(campos[6]) >= sizeof(socio->categoria))
        socio->categoria[0] = '\0';
    else
        copiar_cadena(socio->categoria, campos[6], sizeof(socio->categoria));

    //8.fecha de ultima cuota paga
    if(!texto_a_fecha(campos[7], &socio->fecha_ultima_cuota))
        socio->fecha_ultima_cuota = (t_fecha){0, 0, 0};

    //9.plan
    if(strlen(campos[8]) >= sizeof(socio->plan))
        socio->plan[0] = '\0';
    else
        copiar_cadena(socio->plan, campos[8], sizeof(socio->plan));

    //10.email tutor
    if(strlen(campos[9]) >= sizeof(socio->email_tutor))
        strcpy(socio->email_tutor, "emailinvalido");
    else
        copiar_cadena(socio->email_tutor, campos[9], sizeof(socio->email_tutor));

    //estado inicial
    socio->estado = 'A';


    return OK;
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
    fseek(arch, nro_reg * sizeof(t_socio), SEEK_SET);

    if(fread(s, sizeof(t_socio), 1, arch) != 1)
        return ERROR;

    return OK;
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
    fseek(arch, nro_reg * sizeof(t_socio), SEEK_SET);

    if(fwrite(s, sizeof(t_socio), 1, arch) != 1)
        return ERROR;

    if(fflush(arch) != 0)
        return ERROR;

    return OK;
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
    if(ultima_cuota->anio == fecha_proceso->anio)
    {
        if(ultima_cuota->mes == fecha_proceso->mes)
            return 1;
        else if(ultima_cuota->mes == (fecha_proceso->mes - 1) && fecha_proceso->dia <= DIA_LIMITE_CUOTA)
            return 1;
    }

    //Ultima cuota en diciembre y fecha proceso enero
    else if(ultima_cuota->anio == (fecha_proceso->anio - 1))
    {
        if((fecha_proceso->mes == 1) && (ultima_cuota->mes == 12))
            if(fecha_proceso->dia <= DIA_LIMITE_CUOTA)
                return 1;
    }

    return 0;
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
    t_socio socio = {0};
    t_reg_indice reg;
    char cadena[TAM_INGRESO];
    char dni[TAM_INGRESO];
    char extra;
    char tipo_error[TAM_TIPO_ERROR];

    FILE *arch_bin = fopen(path, "rb+");

    if(!arch_bin)
        return ERROR;

    t_contexto ctx = {indice, arch_bin, fecha_proceso, 0};

    //DNI
    carga_manual("\nDNI: ", cadena, sizeof(cadena));
    strcpy(dni, cadena);
    if(sscanf(cadena, "%ld%c", &socio.dni, &extra) != 1)
    {
        socio.dni = 0;
    }

    //CUIL
    carga_manual("\nCUIL (XY-XXXXXXXX-Z): ", cadena, sizeof(cadena));
    if((strlen(cadena) >= TAM_CUIL) || (sscanf(cadena, "%s", socio.cuil) != 1))
        strcpy(socio.cuil, "00-00000000-0");

    //APELLIDO Y NOMBRE
    carga_manual("\nAPELLIDOS Y NOMBRES: ", cadena, sizeof(cadena));
    if(strlen(cadena) >= TAM_APYN || strlen(cadena) == 0)
        strcpy(socio.apellidos_nombres, "\0");
    else
        strcpy(socio.apellidos_nombres, cadena);

    //FECHA NACIMIENTO
    carga_manual("\nFECHA NACIMIENTO: ", cadena, sizeof(cadena));
    if(sscanf(cadena, "%d/%d/%d%c",
              &socio.fecha_nacimiento.dia, &socio.fecha_nacimiento.mes, &socio.fecha_nacimiento.anio, &extra) != 3)
    {
        socio.fecha_nacimiento.dia = 0;
        socio.fecha_nacimiento.mes = 0;
        socio.fecha_nacimiento.anio = 0;
    }

    //SEXO
    carga_manual("\nSEXO: ", cadena, sizeof(cadena));
    if(sscanf(cadena, "%c %c", &socio.sexo, &extra) != 1)
        socio.sexo = '?';

    //FECHA AFILIACION
    carga_manual("\nFECHA AFILIACION: ", cadena, sizeof(cadena));
    if(sscanf(cadena, "%d/%d/%d%c",
              &socio.fecha_afiliacion.dia, &socio.fecha_afiliacion.mes, &socio.fecha_afiliacion.anio, &extra) != 3)
    {
        socio.fecha_afiliacion.dia = 0;
        socio.fecha_afiliacion.mes = 0;
        socio.fecha_afiliacion.anio = 0;
    }

    //CATEGORIA
    carga_manual("\nCATEGORIA (MENOR/ADULTO): ", cadena, sizeof(cadena));
    if(strlen(cadena) >= TAM_CATEGORIA || strlen(cadena) == 0)
        strcpy(socio.categoria, "\0");
    else
        strcpy(socio.categoria, cadena);

    //FECHA ULTIMA CUOTA
    carga_manual("\nFECHA ULTIMA CUOTA PAGA: ", cadena, sizeof(cadena));
    if(sscanf(cadena, "%d/%d/%d%c",
              &socio.fecha_ultima_cuota.dia, &socio.fecha_ultima_cuota.mes, &socio.fecha_ultima_cuota.anio, &extra) != 3)
    {
        socio.fecha_ultima_cuota.dia = 0;
        socio.fecha_ultima_cuota.mes = 0;
        socio.fecha_ultima_cuota.anio = 0;
    }

    //PLAN
    carga_manual("\nPLAN (BASIC/PREMIUM/VIP/FAMILY): ", cadena, sizeof(cadena));
    if(strlen(cadena) >= TAM_PLAN || strlen(cadena) == 0)
        strcpy(socio.plan, "\0");
    else
        strcpy(socio.plan, cadena);

    //EMAIL TUTOR
    carga_manual("\nEMAIL TUTOR (si es ADULTO no ingrese ninguno): ", cadena, sizeof(cadena));
    if(strlen(cadena) >= TAM_EMAIL)
        strcpy(socio.email_tutor, "emailinvalido");
    else
        strcpy(socio.email_tutor, cadena);


    if(valida_socio_ALU(&socio, &ctx, tipo_error) == ERROR)
    {
        printf("Alta rechazada. %s invalido.\n\n", tipo_error);
        rechazar_socio(&socio, dni, tipo_error, fecha_proceso);
        fclose(arch_bin);
        return ERROR;
    }

    socio.estado = 'A';

    fseek(arch_bin, 0, SEEK_END);
    reg.id = socio.dni;
    reg.nro_reg = (unsigned)(ftell(arch_bin)/sizeof(t_socio));
    indice_insertar_ALU(indice, &reg, sizeof(t_reg_indice), compara_reg_indice_ALU);
    fwrite(&socio, sizeof(t_socio), 1, arch_bin);

    fclose(arch_bin);

    char nacimiento[TAM_FECHA_TXT];
    char afiliacion[TAM_FECHA_TXT];
    char ultima_cuota[TAM_FECHA_TXT];

    sprintf(nacimiento, "%02d/%02d/%04d",
            socio.fecha_nacimiento.dia,
            socio.fecha_nacimiento.mes,
            socio.fecha_nacimiento.anio);

    sprintf(afiliacion, "%02d/%02d/%04d",
            socio.fecha_afiliacion.dia,
            socio.fecha_afiliacion.mes,
            socio.fecha_afiliacion.anio);

    sprintf(ultima_cuota, "%02d/%02d/%04d",
            socio.fecha_ultima_cuota.dia,
            socio.fecha_ultima_cuota.mes,
            socio.fecha_ultima_cuota.anio);

    puts("\n================================================================= "
         "NUEVO SOCIO =================================================================");
    printf("%-10s %-15s %-32s %-12s %-3s %-12s %-10s %-12s %-3s %-11s %s\n",
           "DNI",
           "CUIL",
           "APELLIDOS Y NOMBRES",
           "NACIMIENTO",
           "S",
           "AFILIACION",
           "CATEG.",
           "ULT.CUOTA",
           "E",
           "PLAN",
           "EMAIL TUTOR");
    printf("%-10ld %-15s %-32.32s %-12s %-3c %-12s %-10s %-12s %-3c %-11s %s\n\n",
           socio.dni,
           socio.cuil,
           socio.apellidos_nombres,
           nacimiento,
           socio.sexo,
           afiliacion,
           socio.categoria,
           ultima_cuota,
           socio.estado,
           socio.plan,
           socio.email_tutor);
    puts("=================================================================="
         "=============================================================================");

    return OK;
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
    t_reg_indice reg_buscado;
    char cadena[TAM_INGRESO];
    char mensaje[TAM_INGRESO];
    char extra;

    carga_manual("\nDNI del socio a modificar: ", cadena, sizeof(cadena));
    if(sscanf(cadena, "%ld%c", &reg_buscado.id, &extra) != 1 ||
            indice_buscar_ALU(indice, &reg_buscado, sizeof(t_reg_indice), compara_reg_indice_ALU) == NO_EXISTE)
    {
        puts("Modificacion rechazada. El DNI ingresado no existe o fue dado de baja.");
        return ERROR;
    }

    FILE *arch_bin = fopen(path, "rb+");
    t_socio socio;
    char tipo_error[TAM_TIPO_ERROR];
    t_contexto ctx = {indice, arch_bin, fecha_proceso, reg_buscado.id};

    if(!arch_bin)
        return ERROR;


    if(!socio_leer_ALU(arch_bin, reg_buscado.nro_reg, &socio))
    {
        fclose(arch_bin);
        return ERROR;
    }


    char nacimiento[TAM_FECHA_TXT];
    char afiliacion[TAM_FECHA_TXT];
    char ultima_cuota[TAM_FECHA_TXT];

    sprintf(nacimiento, "%02d/%02d/%04d",
            socio.fecha_nacimiento.dia,
            socio.fecha_nacimiento.mes,
            socio.fecha_nacimiento.anio);

    sprintf(afiliacion, "%02d/%02d/%04d",
            socio.fecha_afiliacion.dia,
            socio.fecha_afiliacion.mes,
            socio.fecha_afiliacion.anio);

    sprintf(ultima_cuota, "%02d/%02d/%04d",
            socio.fecha_ultima_cuota.dia,
            socio.fecha_ultima_cuota.mes,
            socio.fecha_ultima_cuota.anio);

    printf("\nDatos actuales:\n");
    printf("%-10s %-15s %-32s %-12s %-3s %-12s %-10s %-12s %-3s %-12s %s\n",
           "DNI",
           "CUIL",
           "APELLIDOS Y NOMBRES",
           "NACIMIENTO",
           "S",
           "AFILIACION",
           "CATEG.",
           "ULT.CUOTA",
           "E",
           "PLAN",
           "EMAIL TUTOR");
    printf("%-10ld %-15s %-32.32s %-12s %-3c %-12s %-10s %-12s %-3c %-12s %s\n\n",
           socio.dni,
           socio.cuil,
           socio.apellidos_nombres,
           nacimiento,
           socio.sexo,
           afiliacion,
           socio.categoria,
           ultima_cuota,
           socio.estado,
           socio.plan,
           socio.email_tutor);

    printf("(Enter para conservar el valor actual)");

    //CUIL
    sprintf(mensaje, "\nCUIL (XY-XXXXXXXX-Z) [%s]: ", socio.cuil);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if((strlen(cadena) >= TAM_CUIL) || (sscanf(cadena, "%s", socio.cuil) != 1))
            strcpy(socio.cuil, "00-00000000-0");
    }

    //APELLIDOS Y NOMBRES
    sprintf(mensaje, "\nAPELLIDOS Y NOMBRES [%s]: ", socio.apellidos_nombres);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(strlen(cadena) >= TAM_APYN || strlen(cadena) == 0)
            strcpy(socio.apellidos_nombres, "\0");
        else
            strcpy(socio.apellidos_nombres, cadena);
    }

    //FECHA NACIMIENTO
    sprintf(mensaje, "\nFECHA DE NACIMIENTO [%02d/%02d/%04d]: ",
            socio.fecha_nacimiento.dia, socio.fecha_nacimiento.mes, socio.fecha_nacimiento.anio);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(sscanf(cadena, "%d/%d/%d%c",
                  &socio.fecha_nacimiento.dia, &socio.fecha_nacimiento.mes, &socio.fecha_nacimiento.anio, &extra) != 3)
        {
            socio.fecha_nacimiento.dia = 0;
            socio.fecha_nacimiento.mes = 0;
            socio.fecha_nacimiento.anio = 0;
        }
    }

    //SEXO
    sprintf(mensaje, "\nSEXO (H/M) [%c]: ", socio.sexo);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(sscanf(cadena, "%c %c", &socio.sexo, &extra) != 1)
            socio.sexo = '?';
    }

    //FECHA AFILIACION
    sprintf(mensaje, "\nFECHA DE AFILIACION [%02d/%02d/%04d]: ",
            socio.fecha_afiliacion.dia, socio.fecha_afiliacion.mes, socio.fecha_afiliacion.anio);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(sscanf(cadena, "%d/%d/%d%c",
                  &socio.fecha_afiliacion.dia, &socio.fecha_afiliacion.mes, &socio.fecha_afiliacion.anio, &extra) != 3)
        {
            socio.fecha_afiliacion.dia = 0;
            socio.fecha_afiliacion.mes = 0;
            socio.fecha_afiliacion.anio = 0;
        }
    }

    //CATEGORIA
    sprintf(mensaje, "\nCATEGORIA (MENOR/ADULTO) [%s]: ", socio.categoria);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(strlen(cadena) >= TAM_CATEGORIA || strlen(cadena) == 0)
            strcpy(socio.categoria, "\0");
        else
            strcpy(socio.categoria, cadena);
    }

    //FECHA ULTIMA CUOTA
    sprintf(mensaje, "\nFECHA ULTIMA CUOTA PAGA [%02d/%02d/%04d]: ",
            socio.fecha_ultima_cuota.dia, socio.fecha_ultima_cuota.mes, socio.fecha_ultima_cuota.anio);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(sscanf(cadena, "%d/%d/%d%c",
                  &socio.fecha_ultima_cuota.dia, &socio.fecha_ultima_cuota.mes, &socio.fecha_ultima_cuota.anio, &extra) != 3)
        {
            socio.fecha_ultima_cuota.dia = 0;
            socio.fecha_ultima_cuota.mes = 0;
            socio.fecha_ultima_cuota.anio = 0;
        }
    }

    //PLAN
    sprintf(mensaje, "\nPLAN [%s]: ", socio.plan);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(strlen(cadena) >= TAM_PLAN || strlen(cadena) == 0)
            strcpy(socio.plan, "\0");
        else
            strcpy(socio.plan, cadena);
    }

    //EMAIL TUTOR
    sprintf(mensaje, "\nEMAIL TUTOR (vacio si es ADULTO) [%s]: ", socio.email_tutor);
    carga_manual(mensaje, cadena, sizeof(cadena));
    if(*cadena != '\0')
    {
        if(strlen(cadena) >= TAM_EMAIL)
            strcpy(socio.email_tutor, "emailinvalido");
        else
            strcpy(socio.email_tutor, cadena);
    }

    if(valida_socio_ALU(&socio, &ctx, tipo_error) == ERROR)
    {
        printf("Modificacion rechazada. %s invalido.\n\n", tipo_error);
        rechazar_socio(&socio, NULL, tipo_error, fecha_proceso);
        fclose(arch_bin);
        return ERROR;
    }

    if(!socio_escribir_ALU(arch_bin, reg_buscado.nro_reg, &socio))
    {
        fclose(arch_bin);
        return ERROR;
    }


    puts("\nSocio modificado.");
    fclose(arch_bin);

    return OK;
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
    char cadena[TAM_INGRESO];
    t_reg_indice reg_eliminar;
    char extra;
    int pos;

    puts("Ingrese el DNI del socio que quiere eliminar.");
    carga_manual("\nDNI: ", cadena, sizeof(cadena));

    if(sscanf(cadena, "%ld%c", &reg_eliminar.id, &extra) != 1 || !validar_dni(reg_eliminar.id))
    {
        puts("El DNI ingresado es invalido.");
        return ERROR;
    }

    pos = indice_buscar_ALU(indice, &reg_eliminar, sizeof(t_reg_indice), compara_reg_indice_ALU);
    if(pos == NO_EXISTE)
    {
        puts("El DNI ingresado no pertenece a un socio registrado o ya fue dado de baja.");
        return ERROR;
    }

    FILE *pf = fopen(path, "r+b");

    if(!pf)
        return ERROR;

    t_socio socio;

    socio_leer_ALU(pf, reg_eliminar.nro_reg, &socio);
    socio.estado = 'B';
    socio_escribir_ALU(pf, reg_eliminar.nro_reg, &socio);

    fclose(pf);

    indice_eliminar_ALU(indice, &reg_eliminar, sizeof(t_reg_indice), compara_reg_indice_ALU);

    return OK;
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
    FILE *arch_socios = fopen(path, "rb");

    if(!arch_socios)
        return ERROR;

    FILE *arch_beta = fopen(path_export, "wb");

    if(!arch_beta)
    {
        fclose(arch_socios);
        return ERROR;
    }

    t_reg_indice *reg = indice->vindice;
    t_socio socio;
    int cant=0;

    for(unsigned i=0; i < indice->cantidad_elementos_actual; i++, reg++)
    {
        fseek(arch_socios, sizeof(t_socio) * reg->nro_reg, SEEK_SET);
        fread(&socio, sizeof(t_socio), 1, arch_socios);

        if(strcmp(socio.plan, "PREMIUM") == 0 || strcmp(socio.plan, "VIP") == 0)
        {
            if(cuota_al_dia_ALU(&socio.fecha_ultima_cuota, fecha_proceso))
            {
                fwrite(&socio, sizeof(t_socio), 1, arch_beta);
                fflush(arch_beta);
                cant++;
            }
        }
    }

    fclose(arch_beta);
    fclose(arch_socios);

    printf("Socios exportados a %s: %d\n", path_export, cant);

    return OK;
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
    char tecla;
    printf("Alta / Modificacion de socio\n\nA - Alta\nM - Modificacion\nV - Volver\n\nPresione la tecla:");
    tecla = getch();

    while((tecla != 'A' && tecla != 'a') && (tecla != 'M' && tecla != 'm') && (tecla != 'V' && tecla != 'v'))
        tecla = getch();

    tecla = TO_UPPER(tecla);

    switch(tecla)
    {
    case 'A':
        socio_alta_manual_ALU(indice, ARCH_SOCIOS_BIN, fecha_proceso);
        break;
    case 'M':
        socio_modificacion_manual_ALU(indice, ARCH_SOCIOS_BIN, fecha_proceso);
        break;
    case 'V':
        putchar('\n');
        break;
    }
}

long obtener_clave_socio(const void *reg)
{
    const t_socio *socio = (const t_socio *)reg;
    return socio->dni;
}

int es_socio_activo(const void *registro)
{
    const t_socio *socio = (const t_socio *)registro;

    return socio->estado == 'A';
}

void rechazar_socio(const void *reg, const char *linea, const char *tipo_error, const t_fecha *fecha_proc)
{
    if(linea && (strcmp(tipo_error, "DNI") == 0 || strcmp(tipo_error, "FORMATO") == 0))
    {
        char dni[TAM_LINEA];
        strcpy(dni, linea);
        quitar_salto(dni);
        char *aux;
        strcat(dni, ";");
        aux = strchr(dni, ';');
        *aux = '\0';
        auditoria_registrar_ALU(ARCH_ERRORES, dni, tipo_error, fecha_proc);
    }
    else
    {
        const t_socio *socio = reg;
        char dni_texto[TAM_DNI];
        sprintf(dni_texto, "%ld", socio->dni);
        auditoria_registrar_ALU(ARCH_ERRORES, dni_texto, tipo_error, fecha_proc);
    }
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

static int dni_duplicado(const t_contexto *ctx, long dni)
{
    if(ctx->id_modificando == dni)//si se esta modificando este mismo socio no cuenta como duplicado
        return 0;

    t_reg_indice reg_ind;
    reg_ind.id = dni;

    return (indice_buscar_ALU(ctx->indice, &reg_ind, sizeof(t_reg_indice),compara_reg_indice_ALU) != NO_EXISTE);
}
