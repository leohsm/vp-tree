/*
 * vp_tree.c - Implementacao da biblioteca VP-Tree (Vantage-Point Tree)
 *
 * Os nomes e a ordem dos passos seguem o pseudocodigo do relatorio:
 * Construir, Inserir, Remover, BuscarExato, BuscarFaixa e BuscarVizinho.
 */
#include "vp_tree.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

/* ===================================================================== */
/* Utilitarios internos                                                  */
/* ===================================================================== */

/* Par (ponto, distancia ate o ponto de vantagem) usado na construcao. */
typedef struct {
    const void *p;
    double d;
} Item;

static VPNo *novo_no(const void *ponto)
{
    VPNo *no = (VPNo *)malloc(sizeof *no);
    if (no != NULL) {
        no->ponto = ponto;
        no->mu = 0.0;
        no->interno = NULL;
        no->externo = NULL;
    }
    return no;
}

static bool eh_folha(const VPNo *no)
{
    return no->interno == NULL && no->externo == NULL;
}

/* Libera uma subarvore sem recursao (rotacoes ate o filho interno sumir),
 * para nao estourar a pilha em arvores degeneradas. */
static void liberar_nos(VPNo *no)
{
    while (no != NULL) {
        if (no->interno != NULL) {
            VPNo *filho = no->interno;
            no->interno = filho->externo;
            filho->externo = no;
            no = filho;
        } else {
            VPNo *prox = no->externo;
            free(no);
            no = prox;
        }
    }
}

/* Gerador pseudoaleatorio simples (LCG) com estado na propria arvore,
 * para que os resultados sejam reproduziveis. */
static size_t sortear(unsigned long *semente, size_t max)
{
    *semente = *semente * 1103515245UL + 12345UL;
    return (size_t)((*semente >> 16) % (unsigned long)max);
}

static void imprimir_ponto(const VPTree *t, const void *p, FILE *f)
{
    if (t->imprimir != NULL)
        t->imprimir(p, f);
    else
        fprintf(f, "%p", p);
}

static void recuo(FILE *f, int nivel)
{
    int i;
    for (i = 0; i < nivel; i++)
        fputs("  ", f);
}

/* Selecao do k-esimo menor valor (algoritmo de Wirth / quickselect).
 * Ao final, a[k] e o k-esimo menor e a[0..k-1] <= a[k] <= a[k+1..n-1].
 * Tempo esperado O(n). */
static void selecionar(double *a, size_t n, size_t k)
{
    ptrdiff_t l = 0, m = (ptrdiff_t)n - 1, kk = (ptrdiff_t)k;

    while (l < m) {
        double x = a[kk], tmp;
        ptrdiff_t i = l, j = m;
        do {
            while (a[i] < x) i++;
            while (x < a[j]) j--;
            if (i <= j) {
                tmp = a[i]; a[i] = a[j]; a[j] = tmp;
                i++;
                j--;
            }
        } while (i <= j);
        if (j < kk) l = i;
        if (kk < i) m = j;
    }
}

/* Mediana de n valores (o vetor e reordenado). Para n par, e a media
 * dos dois valores centrais, como no relatorio. */
static double mediana(double *a, size_t n)
{
    size_t k = n / 2, i;
    double alto, baixo;

    selecionar(a, n, k);
    alto = a[k];
    if (n % 2 == 1)
        return alto;

    baixo = a[0];                       /* maior valor da metade inferior */
    for (i = 1; i < k; i++)
        if (a[i] > baixo) baixo = a[i];
    return (baixo + alto) / 2.0;
}

/* ===================================================================== */
/* Criacao e destruicao                                                  */
/* ===================================================================== */

VPTree *vp_criar(VPDistancia dist, VPImprimir imprimir, VPEscolha escolha)
{
    VPTree *t;

    if (dist == NULL)
        return NULL;
    t = (VPTree *)malloc(sizeof *t);
    if (t == NULL)
        return NULL;
    t->raiz = NULL;
    t->tamanho = 0;
    t->dist = dist;
    t->imprimir = imprimir;
    t->escolha = escolha;
    t->semente = 20261102UL;
    t->rastro = NULL;
    return t;
}

void vp_limpar(VPTree *t)
{
    if (t == NULL)
        return;
    liberar_nos(t->raiz);
    t->raiz = NULL;
    t->tamanho = 0;
}

void vp_destruir(VPTree *t)
{
    if (t == NULL)
        return;
    vp_limpar(t);
    free(t);
}

void vp_definir_rastro(VPTree *t, FILE *saida)
{
    if (t != NULL)
        t->rastro = saida;
}

/* ===================================================================== */
/* Construcao (operacao fundamental)                                     */
/* ===================================================================== */

typedef struct {
    VPTree *t;
    Item *aux;      /* buffer para a particao estavel          */
    double *ds;     /* buffer para o calculo da mediana        */
    bool erro;
} Construtor;

/* Escolhe o ponto de vantagem e o coloca na posicao 0 de itens[]. */
static void escolher_vp(Construtor *c, Item *itens, size_t n)
{
    VPTree *t = c->t;
    size_t melhor = 0, cand, ncand, namostra, i;
    double melhor_var = -1.0;
    Item tmp;

    if (n <= 1 || t->escolha == VP_ESCOLHA_PRIMEIRO)
        return;

    if (t->escolha == VP_ESCOLHA_ALEATORIA) {
        melhor = sortear(&t->semente, n);
    } else {
        /* VP_ESCOLHA_DISPERSAO (Yianilos, 1993): sorteia alguns candidatos
         * e fica com o que tem a maior variancia das distancias ate uma
         * amostra de elementos. */
        ncand = n < 5 ? n : 5;
        namostra = n < 25 ? n : 25;
        for (cand = 0; cand < ncand; cand++) {
            size_t idx = sortear(&t->semente, n);
            double soma = 0.0, soma2 = 0.0, var;
            for (i = 0; i < namostra; i++) {
                const void *outro = itens[sortear(&t->semente, n)].p;
                double d = t->dist(itens[idx].p, outro);
                soma += d;
                soma2 += d * d;
            }
            var = soma2 / (double)namostra - (soma / (double)namostra) * (soma / (double)namostra);
            if (var > melhor_var) {
                melhor_var = var;
                melhor = idx;
            }
        }
    }
    tmp = itens[0];
    itens[0] = itens[melhor];
    itens[melhor] = tmp;
}

/* Funcao Construir(Lista S) do relatorio. */
static VPNo *construir_rec(Construtor *c, Item *itens, size_t n, int nivel)
{
    VPTree *t = c->t;
    FILE *r = t->rastro;
    VPNo *no;
    Item *resto;
    size_t m, i, nin, pos;

    if (n == 0 || c->erro)
        return NULL;

    escolher_vp(c, itens, n);
    no = novo_no(itens[0].p);
    if (no == NULL) {
        c->erro = true;
        return NULL;
    }

    resto = itens + 1;
    m = n - 1;
    if (r != NULL) {
        recuo(r, nivel);
        fputs("Construir {", r);
        for (i = 0; i < n; i++) {
            if (i > 0) fputc(' ', r);
            imprimir_ponto(t, itens[i].p, r);
        }
        fputs("}: vp = ", r);
        imprimir_ponto(t, no->ponto, r);
        fputc('\n', r);
    }
    if (m == 0) {                        /* folha */
        no->mu = 0.0;
        return no;
    }

    /* Distancias do ponto de vantagem a todos os demais */
    for (i = 0; i < m; i++) {
        resto[i].d = t->dist(no->ponto, resto[i].p);
        c->ds[i] = resto[i].d;
    }
    no->mu = mediana(c->ds, m);

    /* Particao estavel: S_in (d < mu) seguido de S_out (d >= mu),
     * mantendo a ordem original dentro de cada parte. */
    nin = 0;
    for (i = 0; i < m; i++)
        if (resto[i].d < no->mu) c->aux[nin++] = resto[i];
    pos = nin;
    for (i = 0; i < m; i++)
        if (!(resto[i].d < no->mu)) c->aux[pos++] = resto[i];
    memcpy(resto, c->aux, m * sizeof *resto);

    if (r != NULL) {
        recuo(r, nivel + 1);
        fputs("distancias:", r);
        for (i = 0; i < m; i++) {
            fputc(' ', r);
            imprimir_ponto(t, resto[i].p, r);
            fprintf(r, "=%.2f", resto[i].d);
        }
        fprintf(r, "\n");
        recuo(r, nivel + 1);
        fprintf(r, "mu = mediana = %.2f  ->  interno: %lu ponto(s), externo: %lu ponto(s)\n",
                no->mu, (unsigned long)nin, (unsigned long)(m - nin));
    }

    no->interno = construir_rec(c, resto, nin, nivel + 1);
    no->externo = construir_rec(c, resto + nin, m - nin, nivel + 1);
    return no;
}

/* Constroi uma subarvore a partir de n pontos; NULL + *erro em falta de memoria. */
static VPNo *construir_pontos(VPTree *t, const void *const *pontos, size_t n, bool *erro)
{
    Construtor c;
    Item *itens;
    VPNo *raiz;
    size_t i;

    *erro = false;
    if (n == 0)
        return NULL;

    itens = (Item *)malloc(n * sizeof *itens);
    c.aux = (Item *)malloc(n * sizeof *c.aux);
    c.ds = (double *)malloc(n * sizeof *c.ds);
    if (itens == NULL || c.aux == NULL || c.ds == NULL) {
        free(itens); free(c.aux); free(c.ds);
        *erro = true;
        return NULL;
    }
    for (i = 0; i < n; i++) {
        itens[i].p = pontos[i];
        itens[i].d = 0.0;
    }
    c.t = t;
    c.erro = false;

    raiz = construir_rec(&c, itens, n, 0);
    if (c.erro) {
        liberar_nos(raiz);
        raiz = NULL;
        *erro = true;
    }
    free(itens); free(c.aux); free(c.ds);
    return raiz;
}

bool vp_construir(VPTree *t, const void *const *pontos, size_t n)
{
    bool erro;
    VPNo *raiz;

    if (t == NULL || (n > 0 && pontos == NULL))
        return false;
    raiz = construir_pontos(t, pontos, n, &erro);
    vp_limpar(t);
    if (erro)
        return false;
    t->raiz = raiz;
    t->tamanho = n;
    return true;
}

/* ===================================================================== */
/* Insercao                                                              */
/* ===================================================================== */

bool vp_inserir(VPTree *t, const void *ponto)
{
    FILE *r;
    VPNo *novo, *no;
    double d;

    if (t == NULL)
        return false;
    r = t->rastro;
    novo = novo_no(ponto);
    if (novo == NULL)
        return false;

    if (r != NULL) {
        fputs("Inserir(", r);
        imprimir_ponto(t, ponto, r);
        fputs(")\n", r);
    }

    if (t->raiz == NULL) {
        t->raiz = novo;
        t->tamanho = 1;
        if (r != NULL) fputs("  arvore vazia: o ponto vira a raiz\n", r);
        return true;
    }

    no = t->raiz;
    for (;;) {
        d = t->dist(no->ponto, ponto);

        /* Caso especial: no e uma folha, ainda sem raio definido */
        if (eh_folha(no)) {
            no->mu = d;
            no->externo = novo;
            if (r != NULL) {
                fputs("  folha ", r);
                imprimir_ponto(t, no->ponto, r);
                fprintf(r, ": d = %.2f  ->  mu da folha = %.2f, novo ponto vira filho externo\n", d, d);
            }
            break;
        }

        if (r != NULL) {
            fputs("  visita ", r);
            imprimir_ponto(t, no->ponto, r);
            fprintf(r, ": d = %.2f %s mu = %.2f  ->  %s\n", d, d < no->mu ? "<" : ">=",
                    no->mu, d < no->mu ? "interno" : "externo");
        }

        if (d < no->mu) {
            if (no->interno == NULL) { no->interno = novo; break; }
            no = no->interno;
        } else {
            if (no->externo == NULL) { no->externo = novo; break; }
            no = no->externo;
        }
    }
    t->tamanho++;
    return true;
}

/* ===================================================================== */
/* Remocao                                                               */
/* ===================================================================== */

/* Vetor dinamico simples de ponteiros. */
typedef struct {
    void **v;
    size_t n, cap;
} Vetor;

static bool vetor_add(Vetor *a, void *x)
{
    if (a->n == a->cap) {
        size_t nova = a->cap ? a->cap * 2 : 16;
        void **p = (void **)realloc(a->v, nova * sizeof *p);
        if (p == NULL)
            return false;
        a->v = p;
        a->cap = nova;
    }
    a->v[a->n++] = x;
    return true;
}

/* Coleta (em pre-ordem: no, interno, externo) os pontos dos descendentes
 * de 'alvo', sem incluir o proprio alvo. Usa pilha explicita. */
static bool coletar_descendentes(const VPNo *alvo, Vetor *pontos)
{
    Vetor pilha = {NULL, 0, 0};
    bool ok = true;

    if (alvo->externo != NULL) ok = ok && vetor_add(&pilha, alvo->externo);
    if (alvo->interno != NULL) ok = ok && vetor_add(&pilha, alvo->interno);
    while (ok && pilha.n > 0) {
        const VPNo *no = (const VPNo *)pilha.v[--pilha.n];
        ok = vetor_add(pontos, (void *)no->ponto);
        if (ok && no->externo != NULL) ok = vetor_add(&pilha, no->externo);
        if (ok && no->interno != NULL) ok = vetor_add(&pilha, no->interno);
    }
    free(pilha.v);
    return ok;
}

bool vp_remover(VPTree *t, const void *ponto, const void **removido)
{
    FILE *r;
    VPNo **link, *alvo, *nova;
    Vetor pontos = {NULL, 0, 0};
    double d;
    bool erro;

    if (t == NULL)
        return false;
    r = t->rastro;
    if (r != NULL) {
        fputs("Remover(", r);
        imprimir_ponto(t, ponto, r);
        fputs(")\n", r);
    }

    /* 1. Localizar o no (busca exata: um unico caminho) */
    link = &t->raiz;
    while (*link != NULL) {
        VPNo *no = *link;
        d = t->dist(no->ponto, ponto);
        if (r != NULL) {
            fputs("  visita ", r);
            imprimir_ponto(t, no->ponto, r);
            if (d == 0.0)
                fputs(": d = 0  ->  encontrado\n", r);
            else
                fprintf(r, ": d = %.2f %s mu = %.2f  ->  %s\n", d, d < no->mu ? "<" : ">=",
                        no->mu, d < no->mu ? "interno" : "externo");
        }
        if (d == 0.0)
            break;
        link = d < no->mu ? &no->interno : &no->externo;
    }
    if (*link == NULL) {
        if (r != NULL) fputs("  ponto nao encontrado\n", r);
        return false;
    }
    alvo = *link;

    /* 2. Coletar os descendentes */
    if (!coletar_descendentes(alvo, &pontos)) {
        free(pontos.v);
        return false;
    }
    if (r != NULL) {
        size_t i;
        fputs("  descendentes coletados: {", r);
        for (i = 0; i < pontos.n; i++) {
            if (i > 0) fputc(' ', r);
            imprimir_ponto(t, pontos.v[i], r);
        }
        fputs("}\n", r);
    }

    /* 3. Reconstruir a subarvore (so libera a antiga depois de conseguir) */
    nova = construir_pontos(t, (const void *const *)pontos.v, pontos.n, &erro);
    free(pontos.v);
    if (erro)
        return false;

    if (removido != NULL)
        *removido = alvo->ponto;
    liberar_nos(alvo);
    *link = nova;
    t->tamanho--;
    return true;
}

/* ===================================================================== */
/* Buscas                                                                */
/* ===================================================================== */

const void *vp_buscar_exato(const VPTree *t, const void *ponto, size_t *calculos)
{
    const VPNo *no;
    size_t c = 0;
    const void *achado = NULL;

    if (t != NULL) {
        no = t->raiz;
        while (no != NULL) {
            double d = t->dist(no->ponto, ponto);
            c++;
            if (d == 0.0) {
                achado = no->ponto;
                break;
            }
            no = d < no->mu ? no->interno : no->externo;
        }
    }
    if (calculos != NULL)
        *calculos = c;
    return achado;
}

/* ---- k vizinhos mais proximos (k = 1 e o vizinho mais proximo) ---- */

typedef struct {
    const VPTree *t;
    const void *q;
    size_t k;
    Item *heap;       /* max-heap pela distancia: heap[0] e o pior candidato */
    size_t n;
    size_t calculos;
} BuscaK;

static void heap_descer(Item *h, size_t n, size_t i)
{
    for (;;) {
        size_t e = 2 * i + 1, dmaior = i;
        Item tmp;
        if (e < n && h[e].d > h[dmaior].d) dmaior = e;
        if (e + 1 < n && h[e + 1].d > h[dmaior].d) dmaior = e + 1;
        if (dmaior == i) return;
        tmp = h[i]; h[i] = h[dmaior]; h[dmaior] = tmp;
        i = dmaior;
    }
}

static void heap_subir(Item *h, size_t i)
{
    while (i > 0) {
        size_t pai = (i - 1) / 2;
        Item tmp;
        if (h[pai].d >= h[i].d) return;
        tmp = h[i]; h[i] = h[pai]; h[pai] = tmp;
        i = pai;
    }
}

static double tau_atual(const BuscaK *b)
{
    return b->n < b->k ? DBL_MAX : b->heap[0].d;
}

/* Funcao BuscarVizinho do relatorio, generalizada para k vizinhos. */
static void knn_rec(BuscaK *b, const VPNo *no, int nivel)
{
    const VPTree *t = b->t;
    FILE *r = t->rastro;
    double d, tau;
    bool melhorou = false;

    if (no == NULL)
        return;

    d = t->dist(b->q, no->ponto);
    b->calculos++;
    if (b->n < b->k) {
        b->heap[b->n].p = no->ponto;
        b->heap[b->n].d = d;
        heap_subir(b->heap, b->n);
        b->n++;
        melhorou = true;
    } else if (d < b->heap[0].d) {
        b->heap[0].p = no->ponto;
        b->heap[0].d = d;
        heap_descer(b->heap, b->n, 0);
        melhorou = true;
    }

    if (r != NULL) {
        recuo(r, nivel);
        fprintf(r, "(%lu) visita ", (unsigned long)b->calculos);
        imprimir_ponto(t, no->ponto, r);
        fprintf(r, ": d = %.2f", d);
        if (melhorou && b->n == b->k)
            fprintf(r, "  ->  tau = %.2f", tau_atual(b));
        else if (melhorou)
            fputs("  ->  candidato", r);
        fputc('\n', r);
    }

    if (eh_folha(no))
        return;

    if (d < no->mu) {                     /* q esta dentro da bola */
        if (d - tau_atual(b) < no->mu)
            knn_rec(b, no->interno, nivel + 1);
        tau = tau_atual(b);
        if (d + tau >= no->mu)
            knn_rec(b, no->externo, nivel + 1);
        else if (r != NULL && no->externo != NULL) {
            recuo(r, nivel + 1);
            fputs("poda: subarvore externa de ", r);
            imprimir_ponto(t, no->ponto, r);
            fprintf(r, " descartada (d + tau = %.2f + %.2f = %.2f < mu = %.2f)\n",
                    d, tau, d + tau, no->mu);
        }
    } else {                              /* q esta fora da bola */
        if (d + tau_atual(b) >= no->mu)
            knn_rec(b, no->externo, nivel + 1);
        tau = tau_atual(b);
        if (d - tau < no->mu)
            knn_rec(b, no->interno, nivel + 1);
        else if (r != NULL && no->interno != NULL) {
            recuo(r, nivel + 1);
            fputs("poda: subarvore interna de ", r);
            imprimir_ponto(t, no->ponto, r);
            fprintf(r, " descartada (d - tau = %.2f - %.2f = %.2f >= mu = %.2f)\n",
                    d, tau, d - tau, no->mu);
        }
    }
}

size_t vp_k_vizinhos(const VPTree *t, const void *q, size_t k,
                     const void **vizinhos, double *distancias, size_t *calculos)
{
    BuscaK b;
    size_t total, i;

    if (calculos != NULL)
        *calculos = 0;
    if (t == NULL || t->raiz == NULL || k == 0 || vizinhos == NULL)
        return 0;
    if (k > t->tamanho)
        k = t->tamanho;

    b.t = t;
    b.q = q;
    b.k = k;
    b.n = 0;
    b.calculos = 0;
    b.heap = (Item *)malloc(k * sizeof *b.heap);
    if (b.heap == NULL)
        return 0;

    if (t->rastro != NULL) {
        fputs("Busca dos ", t->rastro);
        fprintf(t->rastro, "%lu vizinho(s) mais proximo(s) de ", (unsigned long)k);
        imprimir_ponto(t, q, t->rastro);
        fputc('\n', t->rastro);
    }
    knn_rec(&b, t->raiz, 1);

    /* Esvazia a max-heap do fim para o inicio: ordem crescente */
    total = b.n;
    for (i = total; i > 0; i--) {
        vizinhos[i - 1] = b.heap[0].p;
        if (distancias != NULL)
            distancias[i - 1] = b.heap[0].d;
        b.heap[0] = b.heap[b.n - 1];
        b.n--;
        heap_descer(b.heap, b.n, 0);
    }
    if (calculos != NULL)
        *calculos = b.calculos;
    free(b.heap);
    return total;
}

const void *vp_vizinho_mais_proximo(const VPTree *t, const void *q,
                                    double *distancia, size_t *calculos)
{
    const void *viz = NULL;
    double d = 0.0;

    if (vp_k_vizinhos(t, q, 1, &viz, &d, calculos) == 0)
        return NULL;
    if (distancia != NULL)
        *distancia = d;
    return viz;
}

/* ---- busca por faixa ---- */

typedef struct {
    const VPTree *t;
    const void *q;
    double raio;
    const void **saida;
    size_t capacidade;
    size_t total;
    size_t calculos;
} BuscaFaixa;

static void faixa_rec(BuscaFaixa *b, const VPNo *no, int nivel)
{
    const VPTree *t = b->t;
    FILE *r = t->rastro;
    double d;

    if (no == NULL)
        return;
    d = t->dist(b->q, no->ponto);
    b->calculos++;
    if (d <= b->raio) {
        if (b->saida != NULL && b->total < b->capacidade)
            b->saida[b->total] = no->ponto;
        b->total++;
    }
    if (r != NULL) {
        recuo(r, nivel);
        fputs("visita ", r);
        imprimir_ponto(t, no->ponto, r);
        fprintf(r, ": d = %.2f%s\n", d, d <= b->raio ? "  ->  dentro da faixa" : "");
    }
    if (eh_folha(no))
        return;

    if (d - b->raio < no->mu)             /* bola de busca toca o interior */
        faixa_rec(b, no->interno, nivel + 1);
    else if (r != NULL && no->interno != NULL) {
        recuo(r, nivel + 1);
        fprintf(r, "poda: interno (d - r = %.2f >= mu = %.2f)\n", d - b->raio, no->mu);
    }
    if (d + b->raio >= no->mu)            /* bola de busca toca o exterior */
        faixa_rec(b, no->externo, nivel + 1);
    else if (r != NULL && no->externo != NULL) {
        recuo(r, nivel + 1);
        fprintf(r, "poda: externo (d + r = %.2f < mu = %.2f)\n", d + b->raio, no->mu);
    }
}

size_t vp_buscar_faixa(const VPTree *t, const void *q, double raio,
                       const void **saida, size_t capacidade, size_t *calculos)
{
    BuscaFaixa b;

    b.t = t;
    b.q = q;
    b.raio = raio;
    b.saida = saida;
    b.capacidade = capacidade;
    b.total = 0;
    b.calculos = 0;
    if (t != NULL && raio >= 0.0) {
        if (t->rastro != NULL) {
            fputs("Busca por faixa: pontos a distancia <= ", t->rastro);
            fprintf(t->rastro, "%.2f de ", raio);
            imprimir_ponto(t, q, t->rastro);
            fputc('\n', t->rastro);
        }
        faixa_rec(&b, t->raiz, 1);
    }
    if (calculos != NULL)
        *calculos = b.calculos;
    return b.total;
}

/* ===================================================================== */
/* Consultas, validacao e impressao                                      */
/* ===================================================================== */

bool vp_vazia(const VPTree *t)
{
    return t == NULL || t->raiz == NULL;
}

size_t vp_tamanho(const VPTree *t)
{
    return t == NULL ? 0 : t->tamanho;
}

static size_t altura_rec(const VPNo *no)
{
    size_t a, b;
    if (no == NULL)
        return 0;
    a = altura_rec(no->interno);
    b = altura_rec(no->externo);
    return 1 + (a > b ? a : b);
}

size_t vp_altura(const VPTree *t)
{
    return t == NULL ? 0 : altura_rec(t->raiz);
}

/* Confere se todos os pontos da subarvore 'sub' estao do lado certo da
 * bola (centro vp, raio mu). */
static bool lado_correto(const VPTree *t, const void *vp, double mu, const VPNo *sub, bool interno)
{
    double d;
    if (sub == NULL)
        return true;
    d = t->dist(vp, sub->ponto);
    if (interno ? !(d < mu) : !(d >= mu))
        return false;
    return lado_correto(t, vp, mu, sub->interno, interno) &&
           lado_correto(t, vp, mu, sub->externo, interno);
}

static bool valida_rec(const VPTree *t, const VPNo *no, size_t *contagem)
{
    if (no == NULL)
        return true;
    (*contagem)++;
    if (!lado_correto(t, no->ponto, no->mu, no->interno, true)) return false;
    if (!lado_correto(t, no->ponto, no->mu, no->externo, false)) return false;
    return valida_rec(t, no->interno, contagem) && valida_rec(t, no->externo, contagem);
}

bool vp_valida(const VPTree *t)
{
    size_t contagem = 0;
    if (t == NULL)
        return false;
    return valida_rec(t, t->raiz, &contagem) && contagem == t->tamanho;
}

static void imprimir_rec(const VPTree *t, const VPNo *no, FILE *f, char *prefixo,
                         size_t tam, const char *rotulo, bool ultimo, bool raiz)
{
    size_t novo_tam = tam;

    if (!raiz) {
        fputs(prefixo, f);
        fputs(ultimo ? "`-- " : "|-- ", f);
        fputs(rotulo, f);
    }
    if (no == NULL) {
        fputs("-\n", f);
        return;
    }
    imprimir_ponto(t, no->ponto, f);
    if (!eh_folha(no))
        fprintf(f, "  mu=%.2f", no->mu);
    fputc('\n', f);
    if (eh_folha(no))
        return;

    if (!raiz && tam + 5 < 1024) {
        memcpy(prefixo + tam, ultimo ? "    " : "|   ", 5);
        novo_tam = tam + 4;
    }
    imprimir_rec(t, no->interno, f, prefixo, novo_tam, "int: ", false, false);
    imprimir_rec(t, no->externo, f, prefixo, novo_tam, "ext: ", true, false);
    prefixo[tam] = '\0';
}

void vp_imprimir(const VPTree *t, FILE *saida)
{
    char prefixo[1024];

    if (saida == NULL)
        return;
    if (t == NULL || t->raiz == NULL) {
        fputs("(arvore vazia)\n", saida);
        return;
    }
    prefixo[0] = '\0';
    imprimir_rec(t, t->raiz, saida, prefixo, 0, "", true, true);
}
