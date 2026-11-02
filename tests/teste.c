/*
 * teste.c - Testes automatizados da biblioteca VP-Tree
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vp_tree.h"

static int falhas = 0;
static int verificacoes = 0;

#define CHECAR(cond)                                                          \
    do {                                                                      \
        verificacoes++;                                                       \
        if (!(cond)) {                                                        \
            falhas++;                                                         \
            fprintf(stderr, "FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                     \
    } while (0)

#define PERTO(a, b) (fabs((a) - (b)) < 1e-9)

/* Gerador pseudoaleatorio simples e deterministico (LCG). */
static unsigned long semente = 12345UL;
static int aleatorio(int max)
{
    semente = semente * 1103515245UL + 12345UL;
    return (int)((semente >> 16) % (unsigned long)max);
}

/* ---------------- pontos no plano ---------------- */

typedef struct {
    double x, y;
} P2;

static double dist2(const void *a, const void *b)
{
    const P2 *p = (const P2 *)a, *q = (const P2 *)b;
    double dx = p->x - q->x, dy = p->y - q->y;
    return sqrt(dx * dx + dy * dy);
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* ===================================================================== */

static P2 A = {5, 5}, B = {6, 6}, C = {3, 4}, D = {5, 8}, E = {9, 2}, F = {1, 9}, G = {10, 7};

static VPTree *arvore_relatorio(void)
{
    const void *pts[] = {&A, &B, &C, &D, &E, &F, &G};
    VPTree *t = vp_criar(dist2, NULL, VP_ESCOLHA_PRIMEIRO);
    CHECAR(t != NULL);
    CHECAR(vp_construir(t, pts, 7));
    return t;
}

static void teste_exemplos_relatorio(void)
{
    P2 X = {7, 5}, Q = {4, 4}, Z = {0, 0}, Bcopia = {6, 6};
    const void *rem = NULL;
    double d = -1;
    size_t calc = 0;
    VPTree *t = arvore_relatorio();
    VPNo *r = t->raiz;

    /* Figura 2: construcao */
    CHECAR(vp_tamanho(t) == 7 && vp_altura(t) == 3 && vp_valida(t));
    CHECAR(r->ponto == &A && PERTO(r->mu, 4.0));
    CHECAR(r->interno->ponto == &B && PERTO(r->interno->mu, (sqrt(5.0) + sqrt(13.0)) / 2));
    CHECAR(r->interno->interno->ponto == &D && r->interno->interno->mu == 0.0);
    CHECAR(r->interno->externo->ponto == &C);
    CHECAR(r->externo->ponto == &E && PERTO(r->externo->mu, (sqrt(26.0) + sqrt(113.0)) / 2));
    CHECAR(r->externo->interno->ponto == &G);
    CHECAR(r->externo->externo->ponto == &F);

    /* Figura 6: vizinho mais proximo de Q(4,4) com 4 calculos */
    CHECAR(vp_vizinho_mais_proximo(t, &Q, &d, &calc) == &C);
    CHECAR(PERTO(d, 1.0) && calc == 4);

    /* busca exata: um unico caminho */
    CHECAR(vp_buscar_exato(t, &C, &calc) == &C && calc == 3);
    CHECAR(vp_buscar_exato(t, &Z, &calc) == NULL && calc == 3);

    /* Figura 3: insercao de X(7,5) */
    CHECAR(vp_inserir(t, &X));
    {
        VPNo *nd = t->raiz->interno->interno;
        CHECAR(nd->ponto == &D && PERTO(nd->mu, sqrt(13.0)));
        CHECAR(nd->interno == NULL && nd->externo != NULL && nd->externo->ponto == &X);
        CHECAR(PERTO(t->raiz->mu, 4.0) && PERTO(t->raiz->interno->mu, (sqrt(5.0) + sqrt(13.0)) / 2));
    }
    CHECAR(vp_tamanho(t) == 8 && vp_valida(t));

    /* Figura 4: remocao de B (por um ponto igual, nao o mesmo ponteiro) */
    CHECAR(vp_remover(t, &Bcopia, &rem) && rem == &B);
    {
        VPNo *nd = t->raiz->interno;
        CHECAR(nd->ponto == &D && PERTO(nd->mu, (sqrt(13.0) + sqrt(20.0)) / 2));
        CHECAR(nd->interno->ponto == &X && nd->externo->ponto == &C);
        CHECAR(t->raiz->externo->ponto == &E);
    }
    CHECAR(vp_tamanho(t) == 7 && vp_valida(t));
    CHECAR(!vp_remover(t, &B, NULL));
    CHECAR(vp_buscar_exato(t, &B, NULL) == NULL);

    /* faixa e k vizinhos na arvore original */
    vp_destruir(t);
    t = arvore_relatorio();
    {
        const void *res[7], *viz[10];
        double ds[10];
        CHECAR(vp_buscar_faixa(t, &Q, 2.5, res, 7, &calc) == 2 && calc == 4);
        CHECAR(vp_buscar_faixa(t, &Q, 100.0, res, 3, NULL) == 7);  /* capacidade menor */
        CHECAR(vp_k_vizinhos(t, &Q, 10, viz, ds, NULL) == 7);      /* k > tamanho */
        CHECAR(viz[0] == &C && PERTO(ds[0], 1.0) && viz[1] == &A);
        CHECAR(ds[6] >= ds[5] && ds[5] >= ds[4]);
    }
    vp_destruir(t);
}

static void teste_casos_limite(void)
{
    P2 p = {1, 1}, q = {2, 2};
    const void *um[] = {&p}, *dois[] = {&p, &q}, *viz[2];
    VPTree *t = vp_criar(dist2, NULL, VP_ESCOLHA_ALEATORIA);

    CHECAR(vp_criar(NULL, NULL, VP_ESCOLHA_PRIMEIRO) == NULL);
    CHECAR(vp_vazia(t) && vp_tamanho(t) == 0 && vp_altura(t) == 0 && vp_valida(t));
    CHECAR(vp_vizinho_mais_proximo(t, &p, NULL, NULL) == NULL);
    CHECAR(vp_k_vizinhos(t, &p, 3, viz, NULL, NULL) == 0);
    CHECAR(vp_buscar_faixa(t, &p, 10, NULL, 0, NULL) == 0);
    CHECAR(vp_buscar_exato(t, &p, NULL) == NULL);
    CHECAR(!vp_remover(t, &p, NULL));
    CHECAR(vp_construir(t, NULL, 0) && vp_vazia(t));

    CHECAR(vp_construir(t, um, 1) && vp_tamanho(t) == 1 && vp_valida(t));
    CHECAR(vp_remover(t, &p, NULL) && vp_vazia(t) && vp_valida(t));

    CHECAR(vp_construir(t, dois, 2) && vp_tamanho(t) == 2 && vp_valida(t));
    CHECAR(vp_buscar_exato(t, &q, NULL) == &q);
    CHECAR(vp_remover(t, &q, NULL) && vp_tamanho(t) == 1 && vp_valida(t));
    CHECAR(vp_inserir(t, &q) && vp_tamanho(t) == 2 && vp_valida(t));

    vp_limpar(t);
    CHECAR(vp_vazia(t));
    CHECAR(vp_inserir(t, &p) && vp_inserir(t, &q) && vp_valida(t));
    vp_destruir(t);
    vp_destruir(NULL);
}

/* ---- operacoes aleatorias comparadas com uma busca exaustiva ---- */

#define MAXP 6000

static void conferir_buscas(VPTree *t, P2 **ativos, size_t n)
{
    P2 q;
    const void *viz[5], *res[MAXP];
    double ds[5], ref[MAXP], d, r;
    size_t i, k, cont;

    q.x = aleatorio(1100) - 50;
    q.y = aleatorio(1100) - 50;
    for (i = 0; i < n; i++)
        ref[i] = dist2(&q, ativos[i]);
    qsort(ref, n, sizeof ref[0], cmp_double);

    /* vizinho mais proximo */
    {
        const void *p = vp_vizinho_mais_proximo(t, &q, &d, NULL);
        CHECAR(p != NULL && PERTO(d, ref[0]) && PERTO(dist2(&q, p), d));
    }
    /* k vizinhos */
    k = vp_k_vizinhos(t, &q, 5, viz, ds, NULL);
    CHECAR(k == (n < 5 ? n : 5));
    for (i = 0; i < k; i++)
        CHECAR(PERTO(ds[i], ref[i]) && PERTO(dist2(&q, viz[i]), ds[i]));
    /* faixa */
    r = aleatorio(150);
    for (cont = 0; cont < n && ref[cont] <= r; cont++)
        ;
    k = vp_buscar_faixa(t, &q, r, res, MAXP, NULL);
    CHECAR(k == cont);
    for (i = 0; i < k && i < MAXP; i++)
        CHECAR(dist2(&q, res[i]) <= r);
}

static void teste_aleatorio(VPEscolha escolha)
{
    static P2 pool[MAXP];
    P2 *ativos[MAXP];
    const void *pts[MAXP];
    size_t n = 0, usados = 0, i, op;
    VPTree *t = vp_criar(dist2, NULL, escolha);

    /* pontos com coordenadas inteiras distintas */
    while (usados < MAXP) {
        P2 p;
        bool repetido = false;
        p.x = aleatorio(1000);
        p.y = aleatorio(1000);
        for (i = 0; i < usados && !repetido; i++)
            repetido = pool[i].x == p.x && pool[i].y == p.y;
        if (!repetido)
            pool[usados++] = p;
    }
    usados = 0;

    /* construcao inicial com 1500 pontos */
    for (i = 0; i < 1500; i++) {
        ativos[n] = &pool[usados++];
        pts[n] = ativos[n];
        n++;
    }
    CHECAR(vp_construir(t, pts, n) && vp_tamanho(t) == n && vp_valida(t));
    CHECAR(vp_altura(t) <= 12);   /* balanceada: ceil(log2(1501)) = 11 */

    for (op = 0; op < 6000; op++) {
        int tipo = aleatorio(10);
        if (tipo < 3 && usados < MAXP) {                 /* inserir */
            ativos[n] = &pool[usados++];
            CHECAR(vp_inserir(t, ativos[n]));
            n++;
        } else if (tipo < 5 && n > 0) {                  /* remover existente */
            const void *rem = NULL;
            P2 copia;
            size_t j = (size_t)aleatorio((int)n);
            copia = *ativos[j];
            CHECAR(vp_remover(t, &copia, &rem) && rem == ativos[j]);
            ativos[j] = ativos[--n];
        } else if (tipo == 5) {                          /* remover inexistente */
            P2 fora = {-5.0, -7.0};
            CHECAR(!vp_remover(t, &fora, NULL));
        } else if (tipo == 6 && n > 0) {                 /* busca exata */
            size_t j = (size_t)aleatorio((int)n);
            CHECAR(vp_buscar_exato(t, ativos[j], NULL) == ativos[j]);
        } else if (n > 0) {
            conferir_buscas(t, ativos, n);
        }
        if (op % 500 == 0)
            CHECAR(vp_tamanho(t) == n && vp_valida(t));
        if (op == 3000) {                                /* reconstrucao */
            for (i = 0; i < n; i++)
                pts[i] = ativos[i];
            CHECAR(vp_construir(t, pts, n) && vp_valida(t));
        }
    }
    CHECAR(vp_tamanho(t) == n && vp_valida(t));
    vp_destruir(t);
}

/* ---- espaco metrico sem coordenadas: palavras ---- */

static double levenshtein(const void *a, const void *b)
{
    const char *s = (const char *)a, *u = (const char *)b;
    size_t n = strlen(s), m = strlen(u), i, j, ant[32], atual[32];
    for (j = 0; j <= m; j++) ant[j] = j;
    for (i = 1; i <= n; i++) {
        atual[0] = i;
        for (j = 1; j <= m; j++) {
            size_t x = ant[j - 1] + (s[i - 1] != u[j - 1]), y = ant[j] + 1, z = atual[j - 1] + 1;
            size_t mn = x < y ? x : y;
            atual[j] = mn < z ? mn : z;
        }
        memcpy(ant, atual, (m + 1) * sizeof ant[0]);
    }
    return (double)ant[m];
}

static void palavra_aleatoria(char *s)
{
    int len = 3 + aleatorio(8), i;
    for (i = 0; i < len; i++)
        s[i] = (char)('a' + aleatorio(6));
    s[len] = '\0';
}

static void teste_palavras(void)
{
    static char palavras[800][16];
    const void *pts[800];
    size_t i, j;
    VPTree *t = vp_criar(levenshtein, NULL, VP_ESCOLHA_DISPERSAO);

    for (i = 0; i < 800; i++) {
        palavra_aleatoria(palavras[i]);
        pts[i] = palavras[i];
    }
    CHECAR(vp_construir(t, pts, 800) && vp_valida(t));
    for (i = 0; i < 300; i++) {
        char q[16];
        double d, melhor = 1e9;
        size_t cont = 0, k;
        palavra_aleatoria(q);
        for (j = 0; j < 800; j++) {
            double dj = levenshtein(q, palavras[j]);
            if (dj < melhor) melhor = dj;
            if (dj <= 2.0) cont++;
        }
        CHECAR(vp_vizinho_mais_proximo(t, q, &d, NULL) != NULL && d == melhor);
        k = vp_buscar_faixa(t, q, 2.0, NULL, 0, NULL);
        CHECAR(k == cont);
    }
    vp_destruir(t);
}

/* ---- empates: distancias repetidas na fronteira da mediana ---- */

static void teste_empates(void)
{
    /* (a) 2000 pontos identicos: todas as distancias sao 0 */
    {
        static P2 iguais[2000];
        const void *pts[2000];
        size_t i;
        VPTree *t = vp_criar(dist2, NULL, VP_ESCOLHA_PRIMEIRO);
        for (i = 0; i < 2000; i++) {
            iguais[i].x = 1.0;
            iguais[i].y = 1.0;
            pts[i] = &iguais[i];
        }
        CHECAR(vp_construir(t, pts, 2000) && vp_valida(t));
        CHECAR(vp_altura(t) == 11);                 /* ceil(log2(2001)) */
        for (i = 0; i < 2000; i++) {
            P2 q = {1.0, 1.0};
            const void *rem = NULL;
            CHECAR(vp_remover(t, &q, &rem) && rem != NULL);
            if (i % 250 == 0)
                CHECAR(vp_valida(t) && vp_tamanho(t) == 2000 - i - 1);
        }
        CHECAR(vp_vazia(t));
        vp_destruir(t);
    }

    /* (b) centro + 12 pontos inteiros a distancia exatamente 5 */
    {
        static P2 c[13] = {{0, 0}, {5, 0}, {4, 3}, {3, 4}, {0, 5}, {-3, 4}, {-4, 3},
                           {-5, 0}, {-4, -3}, {-3, -4}, {0, -5}, {3, -4}, {4, -3}};
        const void *pts[13], *res[13];
        P2 centro = {0, 0};
        size_t i;
        VPTree *t = vp_criar(dist2, NULL, VP_ESCOLHA_PRIMEIRO);
        for (i = 0; i < 13; i++)
            pts[i] = &c[i];
        CHECAR(vp_construir(t, pts, 13) && vp_valida(t));
        CHECAR(t->raiz->mu == 5.0);
        CHECAR(vp_altura(t) == 4);                  /* 12 -> 6 + 6, nao 0 + 12 */
        for (i = 0; i < 13; i++)
            CHECAR(vp_buscar_exato(t, &c[i], NULL) == &c[i]);
        CHECAR(vp_buscar_faixa(t, &centro, 5.0, res, 13, NULL) == 13);
        CHECAR(vp_buscar_faixa(t, &centro, 4.999, res, 13, NULL) == 1);
        vp_destruir(t);
    }

    /* (c) grade 12x12: muitas distancias iguais, raios inteiros na fronteira */
    {
        static P2 grade[144];
        const void *pts[144], *res[144], *viz[6];
        P2 *ativos[144];
        double ds[6], ref[144];
        size_t n = 144, i, j, rodada;
        int e;

        for (e = 0; e < 3; e++) {
            VPTree *t = vp_criar(dist2, NULL, (VPEscolha)e);
            for (i = 0; i < 144; i++) {
                grade[i].x = (double)(i % 12);
                grade[i].y = (double)(i / 12);
                pts[i] = &grade[i];
                ativos[i] = &grade[i];
            }
            n = 144;
            CHECAR(vp_construir(t, pts, n) && vp_valida(t) && vp_altura(t) == 8);
            for (rodada = 0; rodada < 144; rodada++) {
                P2 q;
                size_t k, cont;
                double r = (double)aleatorio(6);
                q.x = (double)aleatorio(14) - 1;
                q.y = (double)aleatorio(14) - 1;
                for (i = 0; i < n; i++)
                    ref[i] = dist2(&q, ativos[i]);
                qsort(ref, n, sizeof ref[0], cmp_double);
                for (cont = 0; cont < n && ref[cont] <= r; cont++)
                    ;
                CHECAR(vp_buscar_faixa(t, &q, r, res, 144, NULL) == cont);
                k = vp_k_vizinhos(t, &q, 6, viz, ds, NULL);
                for (j = 0; j < k; j++)
                    CHECAR(PERTO(ds[j], ref[j]));
                for (j = 0; j < n; j++)
                    CHECAR(vp_buscar_exato(t, ativos[j], NULL) == ativos[j]);
                /* remove um ponto aleatorio e, as vezes, o reinsere */
                j = (size_t)aleatorio((int)n);
                {
                    const void *rem = NULL;
                    P2 *p = ativos[j];
                    CHECAR(vp_remover(t, p, &rem) && rem == p);
                    if (aleatorio(2)) {
                        CHECAR(vp_inserir(t, p));
                    } else {
                        ativos[j] = ativos[--n];
                    }
                }
                CHECAR(vp_tamanho(t) == n && vp_valida(t));
                if (n == 0)
                    break;
            }
            vp_destruir(t);
        }
    }
}

/* ---- arvore degenerada: insercoes em linha reta ---- */

static void teste_degenerada(void)
{
    const size_t n = 20000;
    P2 *pts = (P2 *)malloc(n * sizeof *pts);
    P2 alvo;
    size_t i, calc;
    const void *rem;
    VPTree *t = vp_criar(dist2, NULL, VP_ESCOLHA_PRIMEIRO);

    CHECAR(pts != NULL);
    for (i = 0; i < n; i++) {
        pts[i].x = (double)i;
        pts[i].y = 0.0;
        CHECAR(vp_inserir(t, &pts[i]));
    }
    CHECAR(vp_tamanho(t) == n);
    /* cada insercao vai para o externo da folha: altura n (busca exata iterativa) */
    CHECAR(vp_buscar_exato(t, &pts[n - 1], &calc) == &pts[n - 1] && calc == n);

    /* remover a raiz reconstroi a arvore inteira, agora balanceada */
    alvo = pts[0];
    CHECAR(vp_remover(t, &alvo, &rem) && rem == &pts[0]);
    CHECAR(vp_tamanho(t) == n - 1 && vp_altura(t) <= 16 && vp_valida(t));
    CHECAR(vp_buscar_exato(t, &pts[n - 1], &calc) == &pts[n - 1] && calc <= 16);
    vp_destruir(t);   /* liberacao iterativa */
    free(pts);
}

int main(void)
{
    teste_exemplos_relatorio();
    teste_casos_limite();
    teste_aleatorio(VP_ESCOLHA_PRIMEIRO);
    teste_aleatorio(VP_ESCOLHA_ALEATORIA);
    teste_aleatorio(VP_ESCOLHA_DISPERSAO);
    teste_palavras();
    teste_empates();
    teste_degenerada();

    printf("%d verificacoes, %d falha(s)\n", verificacoes, falhas);
    if (falhas == 0)
        printf("Todos os testes passaram.\n");
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
