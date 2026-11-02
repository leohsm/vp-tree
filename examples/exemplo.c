/*
 * exemplo.c - Programa de exemplo da biblioteca VP-Tree
 *
 * Uso:
 *   ./exemplo        executa a demonstracao guiada (exemplos do relatorio)
 *   ./exemplo -i     abre o menu interativo
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vp_tree.h"

/* ===================================================================== */
/* Pontos no plano (distancia euclidiana)                                */
/* ===================================================================== */

typedef struct {
    char nome[16];
    double x, y;
} Ponto2D;

static double dist_euclidiana(const void *a, const void *b)
{
    const Ponto2D *p = (const Ponto2D *)a, *q = (const Ponto2D *)b;
    double dx = p->x - q->x, dy = p->y - q->y;
    return sqrt(dx * dx + dy * dy);
}

static void imprimir_ponto(const void *p, FILE *f)
{
    const Ponto2D *q = (const Ponto2D *)p;
    fprintf(f, "%s(%g,%g)", q->nome, q->x, q->y);
}

/* Pontos do exemplo do relatorio (Figura 2). */
static Ponto2D PONTOS_RELATORIO[] = {
    {"A", 5, 5}, {"B", 6, 6}, {"C", 3, 4}, {"D", 5, 8},
    {"E", 9, 2}, {"F", 1, 9}, {"G", 10, 7}
};
#define N_RELATORIO (sizeof PONTOS_RELATORIO / sizeof PONTOS_RELATORIO[0])

/* ===================================================================== */
/* Utilitarios                                                           */
/* ===================================================================== */

static void titulo(const char *texto)
{
    printf("\n============================================================\n");
    printf(" %s\n", texto);
    printf("============================================================\n");
}

static void mostrar(const VPTree *t)
{
    vp_imprimir(t, stdout);
    printf("  [tamanho=%lu  altura=%lu  valida=%s]\n",
           (unsigned long)vp_tamanho(t), (unsigned long)vp_altura(t),
           vp_valida(t) ? "sim" : "NAO");
}

static void sair_sem_memoria(void)
{
    fprintf(stderr, "Erro: memoria insuficiente.\n");
    exit(EXIT_FAILURE);
}

static VPTree *criar_ou_sair(VPDistancia d, VPImprimir imp, VPEscolha e)
{
    VPTree *t = vp_criar(d, imp, e);
    if (t == NULL)
        sair_sem_memoria();
    return t;
}

static VPTree *arvore_do_relatorio(bool rastro)
{
    const void *pts[N_RELATORIO];
    size_t i;
    VPTree *t = criar_ou_sair(dist_euclidiana, imprimir_ponto, VP_ESCOLHA_PRIMEIRO);

    for (i = 0; i < N_RELATORIO; i++)
        pts[i] = &PONTOS_RELATORIO[i];
    if (rastro)
        vp_definir_rastro(t, stdout);
    if (!vp_construir(t, pts, N_RELATORIO))
        sair_sem_memoria();
    vp_definir_rastro(t, NULL);
    return t;
}

/* ===================================================================== */
/* Demonstracao guiada                                                   */
/* ===================================================================== */

static void demo_construcao_insercao_remocao(void)
{
    static Ponto2D X = {"X", 7, 5};
    VPTree *t;
    const void *removido = NULL;
    Ponto2D alvo = {"B", 6, 6};

    titulo("1. Construcao da VP-Tree do exemplo (Figura 2 do relatorio)");
    printf("Pontos: A(5,5) B(6,6) C(3,4) D(5,8) E(9,2) F(1,9) G(10,7)\n");
    printf("Ponto de vantagem: o primeiro elemento de cada lista (didatico)\n\n");
    t = arvore_do_relatorio(true);
    printf("\n");
    mostrar(t);
    printf("\nEsperado: raiz A (mu=4.00); B (mu=2.92) com D interno e C externo;\n"
           "E (mu=7.86) com G interno e F externo.\n");

    titulo("2. Insercao de X(7,5) (Figura 3 do relatorio)");
    vp_definir_rastro(t, stdout);
    if (!vp_inserir(t, &X))
        sair_sem_memoria();
    vp_definir_rastro(t, NULL);
    printf("\n");
    mostrar(t);
    printf("\nEsperado: a folha D recebe mu=3.61 e X vira seu filho externo.\n");

    titulo("3. Remocao de B (Figura 4 do relatorio)");
    vp_definir_rastro(t, stdout);
    if (vp_remover(t, &alvo, &removido)) {
        vp_definir_rastro(t, NULL);
        printf("\nRemovido: ");
        imprimir_ponto(removido, stdout);
        printf("\n\n");
    }
    mostrar(t);
    printf("\nEsperado: a subarvore de B e reconstruida com D como ponto de\n"
           "vantagem (mu=4.04), X interno e C externo.\n");

    vp_destruir(t);
}

static void demo_buscas(void)
{
    Ponto2D Q = {"Q", 4, 4}, Z = {"Z", 0, 0};
    VPTree *t = arvore_do_relatorio(false);
    const void *viz, *k3[3], *faixa[N_RELATORIO];
    double d, dk[3];
    size_t calc, n, i;

    titulo("4. Vizinho mais proximo de Q(4,4) (Figura 6 do relatorio)");
    mostrar(t);
    printf("\n");
    vp_definir_rastro(t, stdout);
    viz = vp_vizinho_mais_proximo(t, &Q, &d, &calc);
    vp_definir_rastro(t, NULL);
    printf("\nResultado: ");
    imprimir_ponto(viz, stdout);
    printf(" a distancia %.2f, com %lu calculos de distancia (busca exaustiva: %lu).\n",
           d, (unsigned long)calc, (unsigned long)vp_tamanho(t));
    printf("Esperado: C(3,4), distancia 1.00, 4 calculos; E, G e F podados.\n");

    titulo("5. Busca exata, k vizinhos e busca por faixa");
    viz = vp_buscar_exato(t, &PONTOS_RELATORIO[2], &calc);
    printf("BuscarExato(C(3,4)): %s com %lu calculo(s) (um unico caminho)\n",
           viz ? "encontrado" : "nao encontrado", (unsigned long)calc);
    viz = vp_buscar_exato(t, &Z, &calc);
    printf("BuscarExato(Z(0,0)): %s com %lu calculo(s)\n\n",
           viz ? "encontrado" : "nao encontrado", (unsigned long)calc);

    n = vp_k_vizinhos(t, &Q, 3, k3, dk, &calc);
    printf("3 vizinhos mais proximos de Q(4,4) (%lu calculos):\n", (unsigned long)calc);
    for (i = 0; i < n; i++) {
        printf("  %lu. ", (unsigned long)(i + 1));
        imprimir_ponto(k3[i], stdout);
        printf("  d = %.2f\n", dk[i]);
    }
    printf("\n");

    vp_definir_rastro(t, stdout);
    n = vp_buscar_faixa(t, &Q, 2.5, faixa, N_RELATORIO, &calc);
    vp_definir_rastro(t, NULL);
    printf("\n%lu ponto(s) a distancia <= 2.5 de Q (%lu calculos):", (unsigned long)n,
           (unsigned long)calc);
    for (i = 0; i < n; i++) {
        printf(" ");
        imprimir_ponto(faixa[i], stdout);
    }
    printf("\n");
    vp_destruir(t);
}

/* ---- Espaco metrico sem coordenadas: palavras e distancia de edicao ---- */

static double dist_levenshtein(const void *a, const void *b)
{
    const char *s = (const char *)a, *u = (const char *)b;
    size_t n = strlen(s), m = strlen(u), i, j;
    size_t ant[64], atual[64];

    if (n > 63) n = 63;
    if (m > 63) m = 63;
    for (j = 0; j <= m; j++)
        ant[j] = j;
    for (i = 1; i <= n; i++) {
        atual[0] = i;
        for (j = 1; j <= m; j++) {
            size_t sub = ant[j - 1] + (s[i - 1] != u[j - 1]);
            size_t del = ant[j] + 1, ins = atual[j - 1] + 1;
            size_t menor = sub < del ? sub : del;
            atual[j] = menor < ins ? menor : ins;
        }
        memcpy(ant, atual, (m + 1) * sizeof ant[0]);
    }
    return (double)ant[m];
}

static void imprimir_palavra(const void *p, FILE *f)
{
    fputs((const char *)p, f);
}

static void demo_palavras(void)
{
    static const char *dicionario[] = {
        "arvore", "grafo", "vertice", "aresta", "caminho", "ciclo", "folha", "raiz",
        "altura", "nivel", "binaria", "busca", "heap", "fila", "pilha", "lista",
        "vetor", "matriz", "ponteiro", "memoria", "distancia", "metrica", "espaco",
        "vizinho", "consulta", "particao", "mediana", "raio", "ponto", "vantagem",
        "balanceada", "insercao", "remocao", "algoritmo", "complexidade", "recursao",
        "dijkstra", "prim", "kruskal", "largura", "profundidade", "ordenacao"
    };
    const size_t n = sizeof dicionario / sizeof dicionario[0];
    const char *consultas[] = {"arvroe", "grafu", "distansia", "vizino", "mediano"};
    const void *res[16];
    double d;
    size_t calc, i, j, k;
    VPTree *t = criar_ou_sair(dist_levenshtein, imprimir_palavra, VP_ESCOLHA_DISPERSAO);

    titulo("6. Espaco metrico sem coordenadas: corretor ortografico");
    printf("%lu palavras indexadas pela distancia de edicao (Levenshtein).\n",
           (unsigned long)n);
    printf("Uma KD-Tree nao funcionaria aqui: palavras nao tem coordenadas.\n\n");
    if (!vp_construir(t, (const void *const *)dicionario, n))
        sair_sem_memoria();

    for (i = 0; i < sizeof consultas / sizeof consultas[0]; i++) {
        const void *viz = vp_vizinho_mais_proximo(t, consultas[i], &d, &calc);
        printf("\"%s\" -> voce quis dizer \"%s\"? (distancia %.0f, %lu de %lu comparacoes)\n",
               consultas[i], (const char *)viz, d, (unsigned long)calc, (unsigned long)n);
    }

    k = vp_buscar_faixa(t, "raia", 2.0, res, 16, &calc);
    printf("\nPalavras a no maximo 2 edicoes de \"raia\" (%lu comparacoes):",
           (unsigned long)calc);
    for (j = 0; j < k && j < 16; j++)
        printf(" %s", (const char *)res[j]);
    printf("\n");
    vp_destruir(t);
}

/* ---- Desempenho e maldicao da dimensionalidade ---- */

static size_t g_dim = 2;

static double dist_vetor(const void *a, const void *b)
{
    const double *p = (const double *)a, *q = (const double *)b;
    double s = 0.0;
    size_t i;
    for (i = 0; i < g_dim; i++) {
        double d = p[i] - q[i];
        s += d * d;
    }
    return sqrt(s);
}

static unsigned long g_semente = 777UL;
static double aleatorio01(void)
{
    g_semente = g_semente * 1103515245UL + 12345UL;
    return (double)((g_semente >> 16) & 0x7FFF) / 32768.0 +
           (double)((g_semente >> 4) & 0xFFF) / (32768.0 * 4096.0);
}

static void demo_desempenho(void)
{
    const size_t n = 50000, nconsultas = 200;
    const size_t dims[] = {2, 4, 8, 16, 32};
    size_t di, i, j;

    titulo("7. Desempenho: calculos de distancia por consulta (n = 50000)");
    printf("Pontos aleatorios uniformes em [0,1]^D, vizinho mais proximo.\n");
    printf("Busca exaustiva: %lu calculos por consulta.\n\n", (unsigned long)n);
    printf("   D | altura | construcao (s) | calculos/consulta | %% da base\n");
    printf("-----+--------+----------------+-------------------+----------\n");

    for (di = 0; di < sizeof dims / sizeof dims[0]; di++) {
        double *dados, *consulta;
        const void **pts;
        VPTree *t;
        clock_t ini;
        double seg;
        unsigned long total = 0;

        g_dim = dims[di];
        dados = (double *)malloc(n * g_dim * sizeof *dados);
        consulta = (double *)malloc(g_dim * sizeof *consulta);
        pts = (const void **)malloc(n * sizeof *pts);
        if (dados == NULL || consulta == NULL || pts == NULL)
            sair_sem_memoria();
        for (i = 0; i < n * g_dim; i++)
            dados[i] = aleatorio01();
        for (i = 0; i < n; i++)
            pts[i] = dados + i * g_dim;

        t = criar_ou_sair(dist_vetor, NULL, VP_ESCOLHA_DISPERSAO);
        ini = clock();
        if (!vp_construir(t, pts, n))
            sair_sem_memoria();
        seg = (double)(clock() - ini) / CLOCKS_PER_SEC;

        for (i = 0; i < nconsultas; i++) {
            size_t calc;
            for (j = 0; j < g_dim; j++)
                consulta[j] = aleatorio01();
            vp_vizinho_mais_proximo(t, consulta, NULL, &calc);
            total += (unsigned long)calc;
        }
        printf("  %2lu | %6lu | %14.3f | %17.0f | %7.1f%%\n", (unsigned long)g_dim,
               (unsigned long)vp_altura(t), seg, (double)total / nconsultas,
               100.0 * (double)total / nconsultas / (double)n);

        vp_destruir(t);
        free(pts);
        free(consulta);
        free(dados);
    }
    printf("\nEm poucas dimensoes a poda descarta quase toda a arvore; com o aumento\n"
           "de D as distancias se concentram e a busca se aproxima da exaustiva\n"
           "(maldicao da dimensionalidade), como discutido no relatorio.\n");
}

/* ===================================================================== */
/* Menu interativo                                                       */
/* ===================================================================== */

typedef struct {
    Ponto2D **v;
    size_t n, cap;
} Colecao;

static Ponto2D *colecao_add(Colecao *c, const char *nome, double x, double y)
{
    Ponto2D *p;
    if (c->n == c->cap) {
        size_t nova = c->cap ? c->cap * 2 : 16;
        Ponto2D **v = (Ponto2D **)realloc(c->v, nova * sizeof *v);
        if (v == NULL)
            sair_sem_memoria();
        c->v = v;
        c->cap = nova;
    }
    p = (Ponto2D *)malloc(sizeof *p);
    if (p == NULL)
        sair_sem_memoria();
    strncpy(p->nome, nome, sizeof p->nome - 1);
    p->nome[sizeof p->nome - 1] = '\0';
    p->x = x;
    p->y = y;
    c->v[c->n++] = p;
    return p;
}

static void colecao_tirar(Colecao *c, const void *p)
{
    size_t i;
    for (i = 0; i < c->n; i++) {
        if (c->v[i] == p) {
            free(c->v[i]);
            c->v[i] = c->v[--c->n];
            return;
        }
    }
}

static void colecao_limpar(Colecao *c)
{
    size_t i;
    for (i = 0; i < c->n; i++)
        free(c->v[i]);
    c->n = 0;
}

static void reconstruir(VPTree *t, Colecao *c)
{
    if (!vp_construir(t, (const void *const *)c->v, c->n))
        sair_sem_memoria();
}

static bool ler_linha(const char *msg, char *buf, size_t tam)
{
    printf("%s", msg);
    fflush(stdout);
    return fgets(buf, (int)tam, stdin) != NULL;
}

static void menu_interativo(void)
{
    VPTree *t = criar_ou_sair(dist_euclidiana, imprimir_ponto, VP_ESCOLHA_ALEATORIA);
    Colecao col = {NULL, 0, 0};
    char linha[256], nome[16];
    bool rastro = true;
    unsigned contador = 0;

    vp_definir_rastro(t, stdout);
    printf("VP-Tree interativa (pontos no plano, distancia euclidiana).\n");
    printf("Rastro passo a passo ligado (opcao 11 desliga).\n");

    for (;;) {
        int op = -1;
        double x, y, r;
        Ponto2D q = {"q", 0, 0};

        printf("\n 1) Inserir ponto        2) Remover ponto       3) Buscar exato\n"
               " 4) Vizinho mais proximo 5) k vizinhos          6) Busca por faixa\n"
               " 7) Imprimir arvore      8) Reconstruir (balancear)\n"
               " 9) Carregar exemplo do relatorio   10) Gerar pontos aleatorios\n"
               "11) Ligar/desligar rastro            0) Sair\n");
        if (!ler_linha("Opcao: ", linha, sizeof linha))
            break;
        if (sscanf(linha, "%d", &op) != 1)
            continue;
        if (op == 0)
            break;

        switch (op) {
        case 1:
            if (!ler_linha("nome x y: ", linha, sizeof linha)) break;
            if (sscanf(linha, "%15s %lf %lf", nome, &x, &y) != 3) {
                printf("Entrada invalida.\n");
                break;
            }
            if (vp_buscar_exato(t, &(Ponto2D){"", x, y}, NULL) != NULL) {
                printf("Ja existe um ponto em (%g,%g).\n", x, y);
                break;
            }
            if (!vp_inserir(t, colecao_add(&col, nome, x, y)))
                sair_sem_memoria();
            break;
        case 2: case 3: case 4:
            if (!ler_linha("x y: ", linha, sizeof linha)) break;
            if (sscanf(linha, "%lf %lf", &q.x, &q.y) != 2) {
                printf("Entrada invalida.\n");
                break;
            }
            if (op == 2) {
                const void *rem = NULL;
                if (vp_remover(t, &q, &rem)) {
                    printf("Removido: ");
                    imprimir_ponto(rem, stdout);
                    printf("\n");
                    colecao_tirar(&col, rem);
                } else {
                    printf("Ponto nao encontrado.\n");
                }
            } else if (op == 3) {
                size_t calc;
                const void *p = vp_buscar_exato(t, &q, &calc);
                if (p) {
                    printf("Encontrado: ");
                    imprimir_ponto(p, stdout);
                } else {
                    printf("Nao encontrado");
                }
                printf(" (%lu calculos)\n", (unsigned long)calc);
            } else {
                size_t calc;
                double d;
                const void *p = vp_vizinho_mais_proximo(t, &q, &d, &calc);
                if (p == NULL) {
                    printf("Arvore vazia.\n");
                    break;
                }
                printf("Vizinho mais proximo: ");
                imprimir_ponto(p, stdout);
                printf(" d = %.2f (%lu de %lu calculos)\n", d, (unsigned long)calc,
                       (unsigned long)vp_tamanho(t));
            }
            break;
        case 5: {
            const void **viz;
            double *ds;
            unsigned long kl;
            size_t k, n, calc, i;
            if (!ler_linha("x y k: ", linha, sizeof linha)) break;
            if (sscanf(linha, "%lf %lf %lu", &q.x, &q.y, &kl) != 3 || kl == 0 || kl > 100000UL) {
                printf("Entrada invalida.\n");
                break;
            }
            k = (size_t)kl;
            viz = (const void **)malloc(k * sizeof *viz);
            ds = (double *)malloc(k * sizeof *ds);
            if (viz == NULL || ds == NULL)
                sair_sem_memoria();
            n = vp_k_vizinhos(t, &q, k, viz, ds, &calc);
            for (i = 0; i < n; i++) {
                printf("  %lu. ", (unsigned long)(i + 1));
                imprimir_ponto(viz[i], stdout);
                printf("  d = %.2f\n", ds[i]);
            }
            printf("(%lu de %lu calculos)\n", (unsigned long)calc, (unsigned long)vp_tamanho(t));
            free(viz);
            free(ds);
            break;
        }
        case 6: {
            const void *res[64];
            size_t n, calc, i;
            if (!ler_linha("x y raio: ", linha, sizeof linha)) break;
            if (sscanf(linha, "%lf %lf %lf", &q.x, &q.y, &r) != 3 || r < 0) {
                printf("Entrada invalida.\n");
                break;
            }
            n = vp_buscar_faixa(t, &q, r, res, 64, &calc);
            printf("%lu ponto(s) encontrados (%lu calculos):", (unsigned long)n,
                   (unsigned long)calc);
            for (i = 0; i < n && i < 64; i++) {
                printf(" ");
                imprimir_ponto(res[i], stdout);
            }
            printf("%s\n", n > 64 ? " ..." : "");
            break;
        }
        case 7:
            mostrar(t);
            break;
        case 8:
            reconstruir(t, &col);
            mostrar(t);
            break;
        case 9: {
            size_t i;
            colecao_limpar(&col);
            for (i = 0; i < N_RELATORIO; i++)
                colecao_add(&col, PONTOS_RELATORIO[i].nome, PONTOS_RELATORIO[i].x,
                            PONTOS_RELATORIO[i].y);
            t->escolha = VP_ESCOLHA_PRIMEIRO;
            reconstruir(t, &col);
            t->escolha = VP_ESCOLHA_ALEATORIA;
            mostrar(t);
            break;
        }
        case 10: {
            unsigned long n, i;
            if (!ler_linha("quantidade: ", linha, sizeof linha)) break;
            if (sscanf(linha, "%lu", &n) != 1 || n == 0 || n > 1000000UL) {
                printf("Entrada invalida.\n");
                break;
            }
            for (i = 0; i < n; i++) {
                char nm[16];
                sprintf(nm, "P%u", ++contador);
                colecao_add(&col, nm, (double)(rand() % 100), (double)(rand() % 100));
            }
            reconstruir(t, &col);
            printf("%lu pontos gerados; arvore reconstruida (altura %lu).\n",
                   (unsigned long)vp_tamanho(t), (unsigned long)vp_altura(t));
            break;
        }
        case 11:
            rastro = !rastro;
            vp_definir_rastro(t, rastro ? stdout : NULL);
            printf("Rastro %s.\n", rastro ? "ligado" : "desligado");
            break;
        default:
            printf("Opcao invalida.\n");
        }
    }

    vp_destruir(t);
    colecao_limpar(&col);
    free(col.v);
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        menu_interativo();
        return 0;
    }
    demo_construcao_insercao_remocao();
    demo_buscas();
    demo_palavras();
    demo_desempenho();
    printf("\nPara o menu interativo: ./exemplo -i\n");
    return 0;
}
