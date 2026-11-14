/* Proyecto: Dungeon Crawler en consola - "El Descenso al Nucleo"
 * Fecha:            13/11/2026
 * Nombre y Apellido: Jesús Ochoa / Ariam Flores
 * Cedula: 31.828.310 / 31.747.735
 * Numero de Seccion: 3 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================== *
    CONSTANTES Y TIPOS
 * =================================================================== */

#define MAX_ENEMIGOS 100   /* capacidad maxima del bestiario */
#define MAX_OBJETOS  100   /* capacidad maxima del catalogo de objetos */
#define MAX_MOCHILA   50   /* tope fisico del arreglo de la mochila */

/* Archivos de salida / persistencia */
#define ARCHIVO_PARTIDA    "partida_guardata.txt"
#define ARCHIVO_HIGHSCORE  "highscore.txt"
#define MAX_LINEA          300   /* tamaño maximo de una linea leida del highscore */
#define MAX_VICTORIAS_MOSTRAR 5  /* cantidad de victorias que muestra el menu de highscores */

/* Parametros de balance del juego */
#define PROB_DROP_COMUN   40   /* % de que un enemigo Comun suelte un objeto */
#define XP_BASE_NIVEL     40   /* XP para pasar del nivel N al N+1 = 50 * N */
#define BONO_HP           10   /* HP maximo que se gana por nivel */
#define BONO_ATAQUE        5   /* ataque que se gana por nivel */
#define BONO_DEFENSA       4   /* defensa que se gana por nivel */
#define HUIDA_BASE        50   /* % base de huir cuando ataque enemigo = defensa jugador */
#define HUIDA_POR_PUNTO    3   /* % que suma/resta cada punto de diferencia */
#define HUIDA_MIN         10   /* % minimo de huir */
#define HUIDA_MAX         90   /* % maximo de huir */
#define CRITICO_UNA_DE     8   /* golpe critico: 1 de cada 8 (12.5%) */
#define CRITICO_NUM        3   /* el critico multiplica el daño por 3/2 = x1.5 */
#define CRITICO_DEN        2
#define ESQUIVA_UNA_DE    16   /* esquives/evita el ataqu: 1 de cada 16 (6.25%) */

/* resultados de como puede terminar una partida */
#define RESULTADO_DERROTA     0
#define RESULTADO_VICTORIA    1
#define RESULTADO_SUSPENDIDA  2   /* el jugador guardo y salio */

typedef char StringCorto[15];
typedef char StringLargo[100];

/* Objeto del catalogo o de la mochila del jugador */
typedef struct {
    StringLargo nombreObjeto;
    int idObjeto, fuerzaValor;
    StringCorto efecto;
} Objeto;

/* Enemigo del bestiario */
typedef struct {
    int idEnemigo, vidaBase, danoBase, defensaBase;
    StringLargo nombreEnemigo;
    StringCorto tipo;            /* "Comun" o "Raro" */
} Entidad;

/* Estado completo de la partida (todo lo que se guarda en disco). La mochila vive aqui dentro para que guardarPartida y usarObjeto */
typedef struct {
    /* atributos */
    int nivel, hpActual, hpLimite, ataque, defensa, experiencia,
    /* entorno */
    pisoActual, pisosTotales, cuartoActual, cuartosPorPiso,
    /* inventario: espacioInventarioActual = cantidad de objetos en la mochila */
    espacioInventarioActual, espacioInventarioLimite;
    Objeto mochila[MAX_MOCHILA];
    /* estadisticas para el highscore */
    int enemigosDerrotados, objetosConsumidos, cuartosExplorados;
} EstadoJuego;

/* Registro de una partida terminada (una linea de highscore.txt) */
typedef struct {
    StringLargo nombre;
    int victoria;                         /* 1 = victoria, 0 = derrota */
    int pisoMaximo, cuartosExplorados;
    int enemigosDerrotados, objetosConsumidos;
    int nivel, hpFinal, ataque, defensa;
} Estadisticas;

/* ===================================================================
   PROTOTIPOS
 * =================================================================== */

/* Modulo 1: Inicializacion y Archivos Input */
void cargarConfiguracion(EstadoJuego *estado);
void cargarBestiario(Entidad bestiario[], int *total_enemigos);
void cargarObjetos(Objeto catalogo[], int *total_objetos);
int leerLineaNoVacia(FILE *archivo, char destino[], int tamano);

/* Modulo 2: Logica de Exploracion y Navegacion */
void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[], int total_enemigos, Objeto catalogo[], int total_objetos);
Entidad seleccionarEnemigo(Entidad bestiario[], int total_enemigos, int es_cuarto_final);
int cuartosEnPiso(EstadoJuego *estado);
void revisarProgresoPiso(EstadoJuego *estado, int totalCuartos);

/* Modulo 3: Sistema de Combate y Progresion */
void iniciarCombate(EstadoJuego *estado, Entidad *enemigo);
int calcularDano(int ataque_atacante, int defensa_defensor);
int sucedeUnaDe(int n);
int calcularGolpe(int ataque_atacante, int defensa_defensor, int *esquivado, int *critico);
void otorgarRecompensa(EstadoJuego *estado, Objeto catalogo[], int total_objetos, char tipo_enemigo[]);
void verificarSubidaNivel(EstadoJuego *estado);
int umbralExperiencia(int nivel);
int calcularExperiencia(Entidad enemigo);
int calcularProbabilidadHuida(int ataque_enemigo, int defensa_jugador);
int intentarHuir(EstadoJuego *estado, Entidad *enemigo);
void turnoEnemigo(EstadoJuego *estado, Entidad *enemigo);

/* Utilidad de entrada por consola */
int leerOpcion(int minimo, int maximo);
void descartarLinea(void);

/* Modulo 4: Gestion de Recursos */
void usarObjeto(EstadoJuego *estado);
int aplicarEfecto(EstadoJuego *estado, Objeto objeto);
void eliminarObjetoMochila(EstadoJuego *estado, int indice);
void mostrarMochila(EstadoJuego *estado);

/* Modulo 5: Persistencia y Output (Highscore) */
void guardarPartida(EstadoJuego estado);
int cargarPartida(EstadoJuego *estado);
void registrarHighscore(EstadoJuego estado, int victoria);
int existePartidaGuardada(void);
void borrarPartidaGuardada(void);
void mostrarHighscores(void);
void pedirNombre(char destino[], int tamano);
int leerRegistroHighscore(char linea[], Estadisticas *registro);

/* Modulo 6: Menus y flujo del juego */
int elegirOpcionMenuPrincipal(void);
void mostrarEstado(EstadoJuego *estado);
int jugarPartida(EstadoJuego *estado, Entidad bestiario[], int total_enemigos, Objeto catalogo[], int total_objetos);
void terminarPartida(EstadoJuego *estado, int resultado);



int main(){
    int total_enemigos, total_objetos, opcion, resultado;
    int salir = 0;
    EstadoJuego jugador;
    Entidad bestiario[MAX_ENEMIGOS];
    Objeto catalogo[MAX_OBJETOS];

    cargarBestiario(bestiario, &total_enemigos);
    cargarObjetos(catalogo, &total_objetos);

    if(total_enemigos == 0 || total_objetos == 0){
        printf("No se puede iniciar el juego sin enemigos y objetos cargados.\n");
        return 1;
    }

    printf("\n##################################################\n");
    printf("#          EL DESCENSO AL NUCLEO                 #\n");
    printf("##################################################\n");

    while(!salir){
        opcion = elegirOpcionMenuPrincipal();

        if(opcion == 1){
            /* nueva partida: la configuracion fija la semilla y los valores iniciales */
            if(existePartidaGuardada()){
                printf("\n Aviso: ya hay una partida guardada; se reemplaza si guardas\n");
                printf(" o se borra cuando esta partida termine.\n");
            }
            cargarConfiguracion(&jugador);
            resultado = jugarPartida(&jugador, bestiario, total_enemigos, catalogo, total_objetos);
            terminarPartida(&jugador, resultado);
        } else if(opcion == 2){
            /* cargar partida: la configuracion fija la semilla y luego el guardado reemplaza el estado */
            cargarConfiguracion(&jugador);
            if(cargarPartida(&jugador)){
                printf("\n Partida cargada. Continuas donde la dejaste.\n");
                resultado = jugarPartida(&jugador, bestiario, total_enemigos, catalogo, total_objetos);
                terminarPartida(&jugador, resultado);
            } else {
                printf("\n No se pudo cargar la partida: el archivo esta danado.\n");
            }
        } else if(opcion == 3){
            mostrarHighscores();
        } else {
            printf("\n Hasta pronto!\n");
            salir = 1;
        }
    }
    return 0;
}

/* ===================================================================
 *  MODULO 1: INICIALIZACION Y ARCHIVOS INPUT
 * =================================================================== */

/* Lee config.txt (semilla, pisos, cuartos base, HP, ataque, defensa y tamano de mochila), inicializa la semilla aleatoria y deja el estado
listo para una partida nueva (nivel 1, piso 1, cuarto 0). El estado se recibe por referencia. Si el archivo falta o esinvalido, termina el programa. */
void cargarConfiguracion(EstadoJuego *estado){
    FILE *parametros = fopen("config.txt", "r");
    int semilla, leidos;

    if(parametros == NULL){
        printf("Error al abrir el archivo de entrada 'config.txt'\n");
        exit(1);
    }

    leidos = fscanf(parametros, "%d %d %d %d %d %d %d",
        &semilla,
        &((*estado).pisosTotales),
        &((*estado).cuartosPorPiso),
        &((*estado).hpLimite),
        &((*estado).ataque),
        &((*estado).defensa),
        &((*estado).espacioInventarioLimite));
    fclose(parametros);

    if(leidos != 7){
        printf("Error: 'config.txt' incompleto o con formato invalido\n");
        exit(1);
    }
    /* validaciones basicas de los parametros */
    if((*estado).pisosTotales < 1 || (*estado).cuartosPorPiso < 2 ||
       (*estado).hpLimite < 1 || (*estado).espacioInventarioLimite < 1){
        printf("Error: valores invalidos en 'config.txt'\n");
        exit(1);
    }
    if((*estado).espacioInventarioLimite > MAX_MOCHILA){
        (*estado).espacioInventarioLimite = MAX_MOCHILA;
    }

    srand(semilla);

    /* valores de inicio de una partida nueva */
    (*estado).nivel = 1;
    (*estado).experiencia = 0;
    (*estado).hpActual = (*estado).hpLimite;
    (*estado).pisoActual = 1;
    (*estado).cuartoActual = 0;
    (*estado).espacioInventarioActual = 0;   /* mochila vacia */
    (*estado).enemigosDerrotados = 0;
    (*estado).objetosConsumidos = 0;
    (*estado).cuartosExplorados = 0;
}

/* lee la siguiente linea no vacia del archivo y le quita el salto de linea. Permite leer nombres con espacios (ej. "Rata Ruinas").
Devuelve 1 si leyo una linea y 0 si llego al fin del archivo. */
int leerLineaNoVacia(FILE *archivo, char destino[], int tamano){
    int leida = 0;

    while(!leida && fgets(destino, tamano, archivo) != NULL){
        /* quita el salto de linea o retorno de carro del final */
        destino[strcspn(destino, "\r\n")] = '\0';
        if(strlen(destino) > 0){
            leida = 1;
        }
    }
    return leida;
}

/*Lee bestiario.txt. Cada enemigo ocupa dos lineas:
 *   linea 1: Nombre (puede tener espacios)
 *   linea 2: [ID] [Vida] [Dano] [Defensa] [Tipo]
La cantidad de enemigos leidos se devuelve por referencia. */
void cargarBestiario(Entidad bestiario[], int *total_enemigos){
    FILE *archivo = fopen("bestiario.txt", "r");
    int i = 0;

    if(archivo == NULL){
        printf("Error al abrir el archivo de entrada 'bestiario.txt'\n");
        *total_enemigos = 0;
        return;
    }

    while(i < MAX_ENEMIGOS &&
          leerLineaNoVacia(archivo, bestiario[i].nombreEnemigo,
                           sizeof(bestiario[i].nombreEnemigo)) &&
          fscanf(archivo, "%d %d %d %d %14s",
            &(bestiario[i].idEnemigo),
            &(bestiario[i].vidaBase),
            &(bestiario[i].danoBase),
            &(bestiario[i].defensaBase),
            bestiario[i].tipo) == 5){
        i++;
    }
    *total_enemigos = i;
    fclose(archivo);
}

/*Lee objetos.txt. Cada objeto ocupa dos lineas:
 * linea 1: Nombre (puede tener espacios)
 * linea 2: [ID] [Fuerza/Valor] [Tipo_Efecto]
La cantidad de objetos leidos se devuelve por referencia. */
void cargarObjetos(Objeto catalogo[], int *total_objetos){
    FILE *archivo = fopen("objetos.txt", "r");
    int i = 0;

    if(archivo == NULL){
        printf("Error al abrir el archivo de entrada 'objetos.txt'\n");
        *total_objetos = 0;
        return;
    }

    while(i < MAX_OBJETOS &&
          leerLineaNoVacia(archivo, catalogo[i].nombreObjeto,
                           sizeof(catalogo[i].nombreObjeto)) &&
          fscanf(archivo, "%d %d %14s",
            &(catalogo[i].idObjeto),
            &(catalogo[i].fuerzaValor),
            catalogo[i].efecto) == 3){
        i++;
    }
    *total_objetos = i;
    fclose(archivo);
}

/* ===================================================================
    MODULO 2: LOGICA DE EXPLORACION Y NAVEGACION
 * =================================================================== */

/*Devuelve cuantos cuartos tiene el piso actual:
 - piso 1: cuartosPorPiso - 1
 - ultimo piso: cuartosPorPiso + 1 (cuarto extra del encuentro final)
 - pisos intermedios: cuartosPorPiso
 Si solo hay un piso, es a la vez el primero y el ultimo (-1 y +1), y queda con la cantidad base. */
int cuartosEnPiso(EstadoJuego *estado){
    int cuartos = (*estado).cuartosPorPiso;

    if((*estado).pisoActual == 1){
        cuartos = cuartos - 1;
    }
    if((*estado).pisoActual == (*estado).pisosTotales){
        cuartos = cuartos + 1;
    }
    return cuartos;
}

/* Elige al azar un enemigo del bestiario: de tipo Raro si es el cuarto final del piso y de tipo Comun en los demas cuartos.
Devuelve una copia del enemigo, asi el combate puede modificar su vida sin alterar el bestiario. Si no existe ningun enemigo del tipo buscado,
elige cualquiera para que el juego no se bloquee. */
Entidad seleccionarEnemigo(Entidad bestiario[], int total_enemigos, int es_cuarto_final){
    int indicesValidos[MAX_ENEMIGOS];
    int contador = 0;
    int i;

    for(i = 0; i < total_enemigos; i++){
        if(es_cuarto_final){
            if(strcmp(bestiario[i].tipo, "Raro") == 0){
                indicesValidos[contador] = i;
                contador++;
            }
        } else {
            if(strcmp(bestiario[i].tipo, "Comun") == 0){
                indicesValidos[contador] = i;
                contador++;
            }
        }
    }

    if(contador == 0){
        return bestiario[rand() % total_enemigos];
    }
    return bestiario[indicesValidos[rand() % contador]];
}

/* Avanza al siguiente cuarto del piso actual, genera su enemigo y lanza el combate. Segun el resultado:
 - el jugador murio: no se hace nada mas (derrota);
 - el enemigo murio: se otorgan XP y botin, se cuenta el cuarto como explorado y se revisa si el piso quedo completo;
 - el jugador huyo: el cuarto NO se supera, se retrocede el contador para que siga pendiente. */
void avanzarCuarto(EstadoJuego *estado, Entidad bestiario[], int total_enemigos, Objeto catalogo[], int total_objetos){
    int totalCuartos = cuartosEnPiso(estado);
    int es_cuarto_final;
    Entidad enemigo, original;

    (*estado).cuartoActual++;
    es_cuarto_final = ((*estado).cuartoActual == totalCuartos);

    printf("\n==================================================\n");
    printf(" Piso %d de %d | Cuarto %d de %d\n",
        (*estado).pisoActual, (*estado).pisosTotales,
        (*estado).cuartoActual, totalCuartos);
    printf("==================================================\n");

    enemigo = seleccionarEnemigo(bestiario, total_enemigos, es_cuarto_final);
    original = enemigo;   /* copia intacta: el combate modifica la vida de enemigo */

    if(es_cuarto_final){
        printf(" Un enemigo RARO bloquea la salida del piso!\n");
    } else {
        printf(" Un enemigo ha aparecido en el camino!\n");
    }

    iniciarCombate(estado, &enemigo);

    if((*estado).hpActual > 0){
        if(enemigo.vidaBase <= 0){
            /* victoria en el combate */
            (*estado).enemigosDerrotados++;
            (*estado).cuartosExplorados++;
            (*estado).experiencia = (*estado).experiencia + calcularExperiencia(original);
            printf(" Ganas %d puntos de experiencia.\n", calcularExperiencia(original));
            verificarSubidaNivel(estado);
            otorgarRecompensa(estado, catalogo, total_objetos, original.tipo);
            revisarProgresoPiso(estado, totalCuartos);
        } else {
            /* el jugador huyo */
            (*estado).cuartoActual--;
            printf(" Escapas del cuarto, pero el camino sigue bloqueado.\n");
        }
    }
}

/* Si ya se completaron todos los cuartos del piso, desciende al siguiente. Si no quedan mas pisos, anuncia la victoria
(el estado queda con pisoActual > pisosTotales). */
void revisarProgresoPiso(EstadoJuego *estado, int totalCuartos){
    if((*estado).cuartoActual >= totalCuartos){
        printf("\n Has limpiado por completo el piso %d!\n", (*estado).pisoActual);
        (*estado).pisoActual++;
        (*estado).cuartoActual = 0;

        if((*estado).pisoActual > (*estado).pisosTotales){
            printf("\n FELICIDADES! Has superado el ultimo cuarto y conquistado el Nucleo. VICTORIA!\n");
        } else {
            printf(" Descendiendo al piso %d...\n", (*estado).pisoActual);
        }
    }
}

/* ===================================================================
    UTILIDAD DE ENTRADA
   =================================================================== */

/* Descarta lo que quede de la linea actual en la entrada (hasta el salto de linea o el fin de archivo). 
Evita que sobras de un scanf se lean como si fueran la siguiente respuesta. */
void descartarLinea(void){
    int c;

    do{
        c = getchar();
    } while(c != '\n' && c != EOF);
}

/* Pide un entero por consola hasta que este entre minimo y maximo (inclusive). Ignora entradas que no sean numeros.
Si la entrada se termina (fin de archivo) cierra el programa para no ciclar. */
int leerOpcion(int minimo, int maximo){
    int opcion = minimo - 1;
    int leidos;

    do{
        printf(" > ");
        leidos = scanf("%d", &opcion);
        if(leidos == EOF){
            printf("\nEntrada finalizada. Saliendo.\n");
            exit(0);
        }
        descartarLinea();               /* tira lo que sobre en la linea */
        if(leidos != 1){
            opcion = minimo - 1;        /* entrada que no era un numero */
        }
        if(opcion < minimo || opcion > maximo){
            printf(" Ingresa un numero entre %d y %d\n", minimo, maximo);
        }
    } while(opcion < minimo || opcion > maximo);

    return opcion;
}

/* ===================================================================
 *  MODULO 3: SISTEMA DE COMBATE Y PROGRESION
 * =================================================================== */

/* Daño de un golpe normal: ataque menos defensa, con minimo de 1 punto. */
int calcularDano(int ataque_atacante, int defensa_defensor){
    int dano = ataque_atacante - defensa_defensor;

    if(dano < 1){
        dano = 1;
    }
    return dano;
}

/* Devuelve 1 con probabilidad de 1 entre n y 0 en caso contrario. */
int sucedeUnaDe(int n){
    return (rand() % n) == 0;
}

/* Resuelve un golpe completo. Primero el defensor puede esquivarlo (1 de cada ESQUIVA_UNA_DE): en ese caso no hay dano. Si no lo esquiva,
se calcula el dano normal y el atacante puede acertar un critico (1 de cada CRITICO_UNA_DE) que lo multiplica por x1.5. El critico se
redondea hacia arriba para que siempre se note (1 pasa a 2). Devuelve el dano y avisa por referencia si hubo esquiva o critico. 
Sirve igual para el jugador y para el enemigo. */
int calcularGolpe(int ataque_atacante, int defensa_defensor, int *esquivado, int *critico){
    int dano = 0;

    *esquivado = sucedeUnaDe(ESQUIVA_UNA_DE);
    *critico = 0;

    if(!(*esquivado)){
        dano = calcularDano(ataque_atacante, defensa_defensor);
        *critico = sucedeUnaDe(CRITICO_UNA_DE);
        if(*critico){
            dano = (dano * CRITICO_NUM + CRITICO_DEN - 1) / CRITICO_DEN;
        }
    }
    return dano;
}

/* Probablidad (en %) de huir con exito. Parte de HUIDA_BASE y sube o baja HUIDA_POR_PUNTO por cada punto que la defensa del jugador
supere (o quede por debajo) del ataque del enemigo. Se limita al rango [HUIDA_MIN, HUIDA_MAX]. */
int calcularProbabilidadHuida(int ataque_enemigo, int defensa_jugador){
    int prob = HUIDA_BASE + HUIDA_POR_PUNTO * (defensa_jugador - ataque_enemigo);

    if(prob < HUIDA_MIN){
        prob = HUIDA_MIN;
    }
    if(prob > HUIDA_MAX){
        prob = HUIDA_MAX;
    }
    return prob;
}

/* Intenta escapar del combate. Devuelve 1 si lo logra y 0 si falla. */
int intentarHuir(EstadoJuego *estado, Entidad *enemigo){
    int prob = calcularProbabilidadHuida((*enemigo).danoBase, (*estado).defensa);
    int exito = (rand() % 100) < prob;

    printf(" Intentas huir (%d%% de probabilidad)... ", prob);
    if(exito){
        printf("lo logras!\n");
    } else {
        printf("fallas!\n");
    }
    return exito;
}

/* el enemigo ataca al jugador (puede fallar por esquiva o acertar un critico). El HP del jugador nunca baja de 0. */
void turnoEnemigo(EstadoJuego *estado, Entidad *enemigo){
    int esquivado, critico;
    int dano = calcularGolpe((*enemigo).danoBase, (*estado).defensa, &esquivado, &critico);

    if(esquivado){
        printf(" %s te ataca, pero esquivas el golpe!\n", (*enemigo).nombreEnemigo);
    } else {
        (*estado).hpActual = (*estado).hpActual - dano;
        if((*estado).hpActual < 0){
            (*estado).hpActual = 0;
        }
        if(critico){
            printf(" GOLPE CRITICO! ");
        }
        printf(" %s te ataca y te hace %d de dano.\n", (*enemigo).nombreEnemigo, dano);
    }
}

/* Combate por turnos. Cada accion del jugador (atacar, usar la mochila o intentar huir) consume el turno y luego ataca el enemigo, salvo que
este ya haya muerto o el jugador haya escapado. Entrar a la mochila y volver atras sin usar ni soltar nada no consume turno
(se detecta porque la cantidad de objetos no cambia). El combate termina cuando muere el jugador, muere el enemigo o el jugador huye. */
void iniciarCombate(EstadoJuego *estado, Entidad *enemigo){
    int huyo = 0;
    int opcion, dano, objetosAntes, turnoConsumido, esquivado, critico;

    printf("\n *** COMBATE contra %s (%s) ***\n",
        (*enemigo).nombreEnemigo, (*enemigo).tipo);

    while((*estado).hpActual > 0 && (*enemigo).vidaBase > 0 && !huyo){
        printf("\n %s: %d HP | Tu: %d/%d HP (Nivel %d)\n",
            (*enemigo).nombreEnemigo, (*enemigo).vidaBase,
            (*estado).hpActual, (*estado).hpLimite, (*estado).nivel);
        printf(" (1) Atacar\n (2) Mochila\n (3) Huir\n");
        opcion = leerOpcion(1, 3);
        turnoConsumido = 1;

        if(opcion == 1){
            dano = calcularGolpe((*estado).ataque, (*enemigo).defensaBase, &esquivado, &critico);
            if(esquivado){
                printf(" Atacas a %s, pero esquiva el golpe!\n", (*enemigo).nombreEnemigo);
            } else {
                (*enemigo).vidaBase = (*enemigo).vidaBase - dano;
                if((*enemigo).vidaBase < 0){
                    (*enemigo).vidaBase = 0;
                }
                if(critico){
                    printf(" GOLPE CRITICO! ");
                }
                printf(" Atacas a %s y le haces %d de dano.\n", (*enemigo).nombreEnemigo, dano);
            }
        } else if(opcion == 2){
            objetosAntes = (*estado).espacioInventarioActual;
            usarObjeto(estado);
            turnoConsumido = ((*estado).espacioInventarioActual != objetosAntes);
        } else {
            huyo = intentarHuir(estado, enemigo);
        }

        if(turnoConsumido && !huyo && (*enemigo).vidaBase > 0){
            turnoEnemigo(estado, enemigo);
        }
    }

    if((*estado).hpActual <= 0){
        printf("\n Has caido en combate...\n");
    } else if((*enemigo).vidaBase <= 0){
        printf("\n Has derrotado a %s!\n", (*enemigo).nombreEnemigo);
    }
}

/* XP que da un enemigo: la mitad de su vida base mas su dano base. */
int calcularExperiencia(Entidad enemigo){
    return enemigo.vidaBase / 2 + enemigo.danoBase;
}

/* XP acumulada necesaria para pasar del nivel indicado al siguiente. */
int umbralExperiencia(int nivel){
    return XP_BASE_NIVEL * nivel;
}

/* Sube de nivel mientras la experiencia alcance el umbral (puede subir
 * varios niveles seguidos). El sobrante de XP se conserva. Cada nivel
 * aumenta HP maximo, ataque y defensa, y cura lo que sube el HP maximo. */
void verificarSubidaNivel(EstadoJuego *estado){
    while((*estado).experiencia >= umbralExperiencia((*estado).nivel)){
        (*estado).experiencia = (*estado).experiencia - umbralExperiencia((*estado).nivel);
        (*estado).nivel++;
        (*estado).hpLimite = (*estado).hpLimite + BONO_HP;
        (*estado).hpActual = (*estado).hpActual + BONO_HP;
        (*estado).ataque = (*estado).ataque + BONO_ATAQUE;
        (*estado).defensa = (*estado).defensa + BONO_DEFENSA;
        printf(" SUBES AL NIVEL %d! (HP max %d | ATK %d | DEF %d)\n",
            (*estado).nivel, (*estado).hpLimite, (*estado).ataque, (*estado).defensa);
    }
}

/* Botin tras ganar un combate: un enemigo Raro siempre suelta un
 * objeto (100%); uno Comun lo suelta con PROB_DROP_COMUN %. El objeto
 * se elige al azar del catalogo. Si la mochila esta llena, el objeto
 * se pierde. */
void otorgarRecompensa(EstadoJuego *estado, Objeto catalogo[], int total_objetos,
                       char tipo_enemigo[]){
    int suelta, indice;

    if(strcmp(tipo_enemigo, "Raro") == 0){
        suelta = 1;
    } else {
        suelta = (rand() % 100) < PROB_DROP_COMUN;
    }

    if(suelta && total_objetos > 0){
        indice = rand() % total_objetos;
        if((*estado).espacioInventarioActual < (*estado).espacioInventarioLimite){
            (*estado).mochila[(*estado).espacioInventarioActual] = catalogo[indice];
            (*estado).espacioInventarioActual++;
            printf(" El enemigo solto un objeto: %s (mochila %d/%d)\n",
                catalogo[indice].nombreObjeto,
                (*estado).espacioInventarioActual, (*estado).espacioInventarioLimite);
        } else {
            printf(" El enemigo solto %s, pero tu mochila esta llena y se pierde.\n",
                catalogo[indice].nombreObjeto);
        }
    }
}

/* ===================================================================
 *  MODULO 4: GESTION DE RECURSOS
 * =================================================================== */

/* Muestra el contenido de la mochila (numerado desde 1), con el efecto y el valor de cada objeto, y los atributos actuales del jugador. */
void mostrarMochila(EstadoJuego *estado){
    int i;

    printf("\n --- MOCHILA (%d/%d) ---\n", (*estado).espacioInventarioActual,
        (*estado).espacioInventarioLimite);
    printf(" HP %d/%d | ATK %d | DEF %d\n", (*estado).hpActual, (*estado).hpLimite,
        (*estado).ataque, (*estado).defensa);
    for(i = 0; i < (*estado).espacioInventarioActual; i++){
        printf(" (%d) %s [%s %d]\n", i + 1, (*estado).mochila[i].nombreObjeto,
            (*estado).mochila[i].efecto, (*estado).mochila[i].fuerzaValor);
    }
    printf(" (0) Volver\n");
}

/* Quita el objeto de la posicion indicada (base 0) corriendo hacia la izquierda los que le siguen, para que la mochila no quede con huecos. */
void eliminarObjetoMochila(EstadoJuego *estado, int indice){
    int i;

    for(i = indice; i < (*estado).espacioInventarioActual - 1; i++){
        (*estado).mochila[i] = (*estado).mochila[i + 1];
    }
    (*estado).espacioInventarioActual--;
}

/* Ejecuta el efecto del objeto segun su Tipo_Efecto (comparando cadenas):
 *   Cura_Vida    : recupera fuerzaValor de HP sin pasar del maximo
 *   Cura_Max     : recupera todo el HP hasta el maximo
 *   Buff_Dano    : sube el ataque de forma permanente en fuerzaValor
 *   Buff_Defensa : sube la defensa de forma permanente en fuerzaValor
 * Para el buff de dano solo se comparan los primeros 7 caracteres ("Buff_Da") y asi funciona igual si el .txt dice Dano o Daño,
 * sin depender de la codificacion del archivo. Devuelve 1 si el efecto se aplico y 0 si el tipo no se reconoce. */
int aplicarEfecto(EstadoJuego *estado, Objeto objeto){
    int aplicado = 1;
    int curado;

    if(strcmp(objeto.efecto, "Cura_Vida") == 0){
        curado = objeto.fuerzaValor;
        if((*estado).hpActual + curado > (*estado).hpLimite){
            curado = (*estado).hpLimite - (*estado).hpActual;
        }
        (*estado).hpActual = (*estado).hpActual + curado;
        printf(" Usas %s y recuperas %d HP (%d/%d).\n", objeto.nombreObjeto,
            curado, (*estado).hpActual, (*estado).hpLimite);
    } else if(strcmp(objeto.efecto, "Cura_Max") == 0){
        curado = (*estado).hpLimite - (*estado).hpActual;
        (*estado).hpActual = (*estado).hpLimite;
        printf(" Usas %s y recuperas %d HP (%d/%d).\n", objeto.nombreObjeto,
            curado, (*estado).hpActual, (*estado).hpLimite);
    } else if(strncmp(objeto.efecto, "Buff_Da", 7) == 0){
        (*estado).ataque = (*estado).ataque + objeto.fuerzaValor;
        printf(" Usas %s: tu ataque sube +%d (ahora %d).\n", objeto.nombreObjeto,
            objeto.fuerzaValor, (*estado).ataque);
    } else if(strcmp(objeto.efecto, "Buff_Defensa") == 0){
        (*estado).defensa = (*estado).defensa + objeto.fuerzaValor;
        printf(" Usas %s: tu defensa sube +%d (ahora %d).\n", objeto.nombreObjeto,
            objeto.fuerzaValor, (*estado).defensa);
    } else {
        printf(" El efecto '%s' no se reconoce; el objeto no se usa.\n", objeto.efecto);
        aplicado = 0;
    }
    return aplicado;
}

/* Menu de la mochila. El jugador elige un objeto y luego decide si lo usa o lo suelta (o cancela y vuelve a la lista); 
con 0 sale sin hacer nada. Termina tras usar o soltar un objeto, o al volver atras.
Usar un objeto suma a objetosConsumidos; soltarlo no. */
void usarObjeto(EstadoJuego *estado){
    int terminado = 0;
    int seleccion, accion;

    if((*estado).espacioInventarioActual == 0){
        printf("\n Tu mochila esta vacia.\n");
        terminado = 1;
    }

    while(!terminado){
        mostrarMochila(estado);
        seleccion = leerOpcion(0, (*estado).espacioInventarioActual);

        if(seleccion == 0){
            terminado = 1;
        } else {
            printf(" %s: (1) Usar  (2) Soltar  (3) Cancelar\n",
                (*estado).mochila[seleccion - 1].nombreObjeto);
            accion = leerOpcion(1, 3);

            if(accion == 1){
                if(aplicarEfecto(estado, (*estado).mochila[seleccion - 1])){
                    eliminarObjetoMochila(estado, seleccion - 1);
                    (*estado).objetosConsumidos++;
                    terminado = 1;
                }
            } else if(accion == 2){
                printf(" Sueltas %s.\n", (*estado).mochila[seleccion - 1].nombreObjeto);
                eliminarObjetoMochila(estado, seleccion - 1);
                terminado = 1;
            }
        }
    }
}

/* ===================================================================
 *  MODULO 5: PERSISTENCIA Y OUTPUT (HIGHSCORE)
 * =================================================================== */

/* Indica si existe un archivo de partida guardada. */
int existePartidaGuardada(void){
    FILE *archivo = fopen(ARCHIVO_PARTIDA, "r");
    int existe = (archivo != NULL);

    if(existe){
        fclose(archivo);
    }
    return existe;
}

/* Elimina el archivo de partida guardada (se usa al terminar la partida). */
void borrarPartidaGuardada(void){
    remove(ARCHIVO_PARTIDA);
}

/* Vuelca el estado completo de la partida en ARCHIVO_PARTIDA. Formato:
 *   linea 1: nivel hpActual hpLimite ataque defensa experiencia
 *   linea 2: pisoActual pisosTotales cuartoActual cuartosPorPiso
 *   linea 3: espacioInventarioActual espacioInventarioLimite
 *   linea 4: enemigosDerrotados objetosConsumidos cuartosExplorados
luego, por cada objeto de la mochila, dos lineas:
    nombre (puede tener espacios)
    id fuerzaValor efecto */
void guardarPartida(EstadoJuego estado){
    FILE *archivo = fopen(ARCHIVO_PARTIDA, "w");
    int i;

    if(archivo == NULL){
        printf("Error: no se pudo crear el archivo de partida guardada\n");
        return;
    }

    fprintf(archivo, "%d %d %d %d %d %d\n", estado.nivel, estado.hpActual,
        estado.hpLimite, estado.ataque, estado.defensa, estado.experiencia);
    fprintf(archivo, "%d %d %d %d\n", estado.pisoActual, estado.pisosTotales,
        estado.cuartoActual, estado.cuartosPorPiso);
    fprintf(archivo, "%d %d\n", estado.espacioInventarioActual,
        estado.espacioInventarioLimite);
    fprintf(archivo, "%d %d %d\n", estado.enemigosDerrotados,
        estado.objetosConsumidos, estado.cuartosExplorados);

    for(i = 0; i < estado.espacioInventarioActual; i++){
        fprintf(archivo, "%s\n%d %d %s\n", estado.mochila[i].nombreObjeto,
            estado.mochila[i].idObjeto, estado.mochila[i].fuerzaValor,
            estado.mochila[i].efecto);
    }
    fclose(archivo);
    printf(" Partida guardada.\n");
}

/* Lee ARCHIVO_PARTIDA y reanuda la partida. Devuelve 1 si se cargo bien y 0 si el archivo no existe o esta danado. Se lee primero a un estado
temporal y solo si todo es valido se copia a *estado, asi un archivo corrupto no deja la partida a medias. */
int cargarPartida(EstadoJuego *estado){
    FILE *archivo = fopen(ARCHIVO_PARTIDA, "r");
    EstadoJuego temp;
    int valido, i;

    if(archivo == NULL){
        return 0;
    }

    valido = (fscanf(archivo, "%d %d %d %d %d %d", &temp.nivel, &temp.hpActual,
                &temp.hpLimite, &temp.ataque, &temp.defensa, &temp.experiencia) == 6);
    valido = valido && (fscanf(archivo, "%d %d %d %d", &temp.pisoActual,
                &temp.pisosTotales, &temp.cuartoActual, &temp.cuartosPorPiso) == 4);
    valido = valido && (fscanf(archivo, "%d %d", &temp.espacioInventarioActual,
                &temp.espacioInventarioLimite) == 2);
    valido = valido && (fscanf(archivo, "%d %d %d", &temp.enemigosDerrotados,
                &temp.objetosConsumidos, &temp.cuartosExplorados) == 3);

    /* validar rangos antes de usar los valores */
    if(valido){
        valido = (temp.nivel >= 1 && temp.hpLimite >= 1 &&
                  temp.hpActual >= 1 && temp.hpActual <= temp.hpLimite &&
                  temp.pisosTotales >= 1 && temp.cuartosPorPiso >= 2 &&
                  temp.pisoActual >= 1 && temp.pisoActual <= temp.pisosTotales &&
                  temp.cuartoActual >= 0 &&
                  temp.espacioInventarioLimite >= 1 &&
                  temp.espacioInventarioLimite <= MAX_MOCHILA &&
                  temp.espacioInventarioActual >= 0 &&
                  temp.espacioInventarioActual <= temp.espacioInventarioLimite);
    }
    if(valido){
        valido = (temp.cuartoActual < cuartosEnPiso(&temp));
    }

    /* leer los objetos de la mochila */
    for(i = 0; valido && i < temp.espacioInventarioActual; i++){
        valido = leerLineaNoVacia(archivo, temp.mochila[i].nombreObjeto,
                    sizeof(temp.mochila[i].nombreObjeto)) &&
                 (fscanf(archivo, "%d %d %14s", &temp.mochila[i].idObjeto,
                    &temp.mochila[i].fuerzaValor, temp.mochila[i].efecto) == 3);
    }
    fclose(archivo);

    if(valido){
        *estado = temp;
    }
    return valido;
}

/* Pide el nombre del explorador por consola (acepta espacios) y no continua hasta recibir uno no vacio. El caracter '|' se cambia por un
espacio porque se usa como separador en highscore.txt. Si la entrada se termina, usa el nombre "Anonimo". */
void pedirNombre(char destino[], int tamano){
    int i, listo = 0;

    while(!listo){
        printf("\n Ingresa tu nombre de explorador: ");
        if(fgets(destino, tamano, stdin) == NULL){
            strcpy(destino, "Anonimo");
            listo = 1;
        } else {
            destino[strcspn(destino, "\r\n")] = '\0';
            for(i = 0; destino[i] != '\0'; i++){
                if(destino[i] == '|'){
                    destino[i] = ' ';
                }
            }
            listo = (strlen(destino) > 0);
        }
    }
}

/* Pide el nombre y agrega un registro al final de highscore.txt (modo append, conserva los jugadores anteriores). Una linea por partida:
   Nombre | Resultado | Piso P | Cuartos C | Enemigos E | Objetos O |
   Nivel N | HP H | ATK A | DEF D
En una victoria pisoActual queda uno por encima del ultimo piso, por eso el piso maximo se limita a pisosTotales. victoria vale 1 o 0. */
void registrarHighscore(EstadoJuego estado, int victoria){
    FILE *archivo;
    StringLargo nombre;
    int pisoMaximo = estado.pisoActual;

    if(pisoMaximo > estado.pisosTotales){
        pisoMaximo = estado.pisosTotales;
    }

    pedirNombre(nombre, sizeof(nombre));

    archivo = fopen(ARCHIVO_HIGHSCORE, "a");
    if(archivo == NULL){
        printf("Error: no se pudo abrir '%s'\n", ARCHIVO_HIGHSCORE);
        return;
    }
    fprintf(archivo, "%s | %s | Piso %d | Cuartos %d | Enemigos %d | Objetos %d | Nivel %d | HP %d | ATK %d | DEF %d\n",
        nombre, victoria ? "Victoria" : "Derrota", pisoMaximo,
        estado.cuartosExplorados, estado.enemigosDerrotados,
        estado.objetosConsumidos, estado.nivel, estado.hpActual,
        estado.ataque, estado.defensa);
    fclose(archivo);
    printf(" Resultado guardado en %s\n", ARCHIVO_HIGHSCORE);
}

/* Convierte una linea de highscore.txt en un registro. Devuelve 1 si la linea tiene el formato esperado y 0 si no (se ignora). */
int leerRegistroHighscore(char linea[], Estadisticas *registro){
    int largoNombre = (int)strcspn(linea, "|");
    StringCorto resultado;
    int leidos;

    if(linea[largoNombre] == '\0' || largoNombre >= (int)sizeof((*registro).nombre)){
        return 0;
    }

    /* el nombre es todo lo que esta antes del primer '|', sin espacios al final */
    strncpy((*registro).nombre, linea, largoNombre);
    (*registro).nombre[largoNombre] = '\0';
    while(largoNombre > 0 && (*registro).nombre[largoNombre - 1] == ' '){
        largoNombre--;
        (*registro).nombre[largoNombre] = '\0';
    }

    leidos = sscanf(linea + strcspn(linea, "|"),
        "| %14s | Piso %d | Cuartos %d | Enemigos %d | Objetos %d | Nivel %d | HP %d | ATK %d | DEF %d",
        resultado, &(*registro).pisoMaximo, &(*registro).cuartosExplorados,
        &(*registro).enemigosDerrotados, &(*registro).objetosConsumidos,
        &(*registro).nivel, &(*registro).hpFinal, &(*registro).ataque,
        &(*registro).defensa);
    if(leidos != 9){
        return 0;
    }
    (*registro).victoria = (strcmp(resultado, "Victoria") == 0);
    return 1;
}

/* Lee highscore.txt y muestra por consola las ultimas MAX_VICTORIAS_MOSTRAR victorias, de la mas reciente a la mas antigua.
Mientras recorre el archivo conserva solo las ultimas victorias vistas (cuando se llena, descarta la mas antigua). */
void mostrarHighscores(void){
    FILE *archivo = fopen(ARCHIVO_HIGHSCORE, "r");
    char linea[MAX_LINEA];
    Estadisticas ultimas[MAX_VICTORIAS_MOSTRAR];
    Estadisticas registro;
    int total = 0;
    int i;

    printf("\n ===== ULTIMAS VICTORIAS =====\n");
    if(archivo == NULL){
        printf(" Aun no hay partidas registradas.\n");
        return;
    }

    while(fgets(linea, sizeof(linea), archivo) != NULL){
        if(leerRegistroHighscore(linea, &registro) && registro.victoria){
            if(total == MAX_VICTORIAS_MOSTRAR){
                for(i = 0; i < MAX_VICTORIAS_MOSTRAR - 1; i++){
                    ultimas[i] = ultimas[i + 1];
                }
                total--;
            }
            ultimas[total] = registro;
            total++;
        }
    }
    fclose(archivo);

    if(total == 0){
        printf(" Todavia no hay victorias registradas.\n");
    } else {
        for(i = total - 1; i >= 0; i--){
            printf(" %d. %s | Nivel %d | Piso %d | Cuartos %d | Enemigos %d | Objetos %d | HP %d | ATK %d | DEF %d\n",
                total - i, ultimas[i].nombre, ultimas[i].nivel,
                ultimas[i].pisoMaximo, ultimas[i].cuartosExplorados,
                ultimas[i].enemigosDerrotados, ultimas[i].objetosConsumidos,
                ultimas[i].hpFinal, ultimas[i].ataque, ultimas[i].defensa);
        }
    }
}

/* ===================================================================
 *  MODULO 6: MENUS Y FLUJO DEL JUEGO
 * =================================================================== */

/* Muestra el menu de inicio y devuelve la opcion elegida (1 a 4). La opcion 2 (cargar partida) solo se ofrece si existe un archivo de
partida guardada; si el jugador la escribe sin que exista, se le avisa y se vuelve a preguntar. */
int elegirOpcionMenuPrincipal(void){
    int hayPartida = existePartidaGuardada();
    int opcion;

    do{
        printf("\n************\n");
        printf("(1) Empezar nueva partida\n");
        if(hayPartida){
            printf("(2) Cargar partida\n");
        }
        printf("(3) Highscores\n");
        printf("(4) Salir\n");
        printf("************\n");
        opcion = leerOpcion(1, 4);

        if(opcion == 2 && !hayPartida){
            printf(" No hay ninguna partida guardada.\n");
            opcion = 0;
        }
    } while(opcion == 0);

    return opcion;
}

/* Resumen de la situacion del jugador antes de cada decision fuera de combate. */
void mostrarEstado(EstadoJuego *estado){
    printf("\n--------------------------------------------------\n");
    printf(" Nivel %d (XP %d/%d) | HP %d/%d | ATK %d | DEF %d\n",
        (*estado).nivel, (*estado).experiencia,
        umbralExperiencia((*estado).nivel),
        (*estado).hpActual, (*estado).hpLimite,
        (*estado).ataque, (*estado).defensa);
    printf(" Piso %d de %d | Cuartos superados: %d de %d | Mochila %d/%d\n",
        (*estado).pisoActual, (*estado).pisosTotales,
        (*estado).cuartoActual, cuartosEnPiso(estado),
        (*estado).espacioInventarioActual, (*estado).espacioInventarioLimite);
    printf("--------------------------------------------------\n");
}

/* Menu fuera de combate: avanzar, revisar la mochila o guardar y salir. Se repite hasta que la partida termina por victoria o derrota,
o el jugador la suspende. Devuelve RESULTADO_VICTORIA, RESULTADO_DERROTA o RESULTADO_SUSPENDIDA. */
int jugarPartida(EstadoJuego *estado, Entidad bestiario[], int total_enemigos,
                 Objeto catalogo[], int total_objetos){
    int terminada = 0;
    int resultado = RESULTADO_SUSPENDIDA;
    int opcion;

    while(!terminada){
        mostrarEstado(estado);
        printf(" (1) Avanzar al siguiente cuarto\n");
        printf(" (2) Revisar mochila\n");
        printf(" (3) Guardar y salir\n");
        opcion = leerOpcion(1, 3);

        if(opcion == 1){
            avanzarCuarto(estado, bestiario, total_enemigos, catalogo, total_objetos);
            if((*estado).hpActual <= 0){
                resultado = RESULTADO_DERROTA;
                terminada = 1;
            } else if((*estado).pisoActual > (*estado).pisosTotales){
                resultado = RESULTADO_VICTORIA;
                terminada = 1;
            }
        } else if(opcion == 2){
            usarObjeto(estado);
        } else {
            guardarPartida(*estado);
            terminada = 1;
        }
    }
    return resultado;
}

/* Cierra una partida. Si fue suspendida no hace nada (el guardado ya esta hecho).
Si termino en victoria o derrota muestra el resumen, registra el highscore y elimina el archivo de partida guardada. */
void terminarPartida(EstadoJuego *estado, int resultado){
    if(resultado != RESULTADO_SUSPENDIDA){
        printf("\n==================================================\n");
        if(resultado == RESULTADO_VICTORIA){
            printf("                    VICTORIA\n");
        } else {
            printf("                    DERROTA\n");
        }
        printf("==================================================\n");
        printf(" Nivel %d | HP %d/%d | ATK %d | DEF %d\n", (*estado).nivel,
            (*estado).hpActual, (*estado).hpLimite, (*estado).ataque, (*estado).defensa);
        printf(" Enemigos derrotados: %d | Objetos consumidos: %d | Cuartos superados: %d\n",
            (*estado).enemigosDerrotados, (*estado).objetosConsumidos,
            (*estado).cuartosExplorados);

        registrarHighscore(*estado, resultado == RESULTADO_VICTORIA);
        borrarPartidaGuardada();
    }
}
