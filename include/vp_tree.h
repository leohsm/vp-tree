/*
 * vp_tree.h - Biblioteca de VP-Tree (Vantage-Point Tree) em C
 *
 * VP-Tree - Yianilos, 1993 (metric trees: Uhlmann, 1991).
 * Arvore binaria para busca por similaridade em espacos metricos gerais.
 * Cada no guarda um ponto de vantagem (vp) e um raio mu:
 *   - subarvore interna: elementos x com d(vp, x) <  mu
 *   - subarvore externa: elementos x com d(vp, x) >= mu
 * Na construcao, mu e a mediana das distancias (media dos dois valores
 * centrais quando a quantidade e par), o que deixa a arvore balanceada.
 *
 * A biblioteca nao conhece o tipo dos pontos: eles sao ponteiros
 * (const void *) do usuario, comparados apenas pela funcao de distancia,
 * que deve ser uma metrica (em especial, respeitar a desigualdade
 * triangular). Os pontos nao sao copiados nem liberados pela arvore.
 *
 * Complexidade (arvore construida pela mediana):
 *   construir                    : O(n log n)
 *   inserir / buscar_exato       : O(log n)        (pior caso: O(n))
 *   remover (reconstroi subarv.) : O(log n + m log m), m = tam. da subarvore
 *   vizinho / k vizinhos / faixa : O(log n) esperado em baixa dimensao
 *                                  intrinseca (pior caso: O(n))
 *
 * Disciplina: Arvores e Grafos - UTFPR Campus Medianeira
 */
#ifndef VP_TREE_H
#define VP_TREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Funcao de distancia (metrica) entre dois pontos do usuario. */
typedef double (*VPDistancia)(const void *a, const void *b);

/* Funcao opcional para imprimir um ponto (usada em vp_imprimir e no rastro). */
typedef void (*VPImprimir)(const void *ponto, FILE *saida);

/* Estrategia de escolha do ponto de vantagem durante a construcao. */
typedef enum {
    VP_ESCOLHA_PRIMEIRO,   /* primeiro elemento da lista (didatico, igual ao relatorio) */
    VP_ESCOLHA_ALEATORIA,  /* elemento sorteado */
    VP_ESCOLHA_DISPERSAO   /* heuristica de Yianilos: candidato com maior
                              dispersao (variancia) das distancias */
} VPEscolha;

/* No da arvore: o ponto de vantagem, o raio e os dois filhos.
 * Nenhuma informacao de balanceamento. */
typedef struct VPNo {
    const void *ponto;        /* ponto de vantagem (vantage point)        */
    double mu;                /* raio de corte (0 em uma folha)           */
    struct VPNo *interno;     /* elementos com d(ponto, x) <  mu          */
    struct VPNo *externo;     /* elementos com d(ponto, x) >= mu          */
} VPNo;

/* Descritor da arvore. */
typedef struct VPTree {
    VPNo *raiz;
    size_t tamanho;
    VPDistancia dist;
    VPImprimir imprimir;      /* pode ser NULL                            */
    VPEscolha escolha;
    unsigned long semente;    /* gerador usado por VP_ESCOLHA_ALEATORIA   */
    FILE *rastro;             /* se nao for NULL, as operacoes descrevem
                                 cada passo neste arquivo (ex.: stdout)  */
} VPTree;

/* ---------------- Criacao e destruicao ---------------- */

/* Cria uma arvore vazia. 'dist' e obrigatoria; 'imprimir' pode ser NULL.
 * Retorna NULL se faltar memoria ou se 'dist' for NULL. */
VPTree *vp_criar(VPDistancia dist, VPImprimir imprimir, VPEscolha escolha);

/* Libera todos os nos e o descritor (nao libera os pontos). Aceita NULL. */
void vp_destruir(VPTree *t);

/* Remove todos os nos, mantendo o descritor utilizavel. */
void vp_limpar(VPTree *t);

/* Liga/desliga o rastro passo a passo das operacoes (NULL desliga). */
void vp_definir_rastro(VPTree *t, FILE *saida);

/* ---------------- Operacao fundamental ---------------- */

/* Constroi a arvore balanceada a partir de n pontos, substituindo o
 * conteudo atual. O vetor 'pontos' nao e modificado.
 * Retorna false se faltar memoria (a arvore fica vazia). */
bool vp_construir(VPTree *t, const void *const *pontos, size_t n);

/* ---------------- Alteracoes ---------------- */

/* Insere um ponto descendo pela arvore guiado por mu (os raios dos nos
 * internos nunca mudam). Ao chegar em uma folha, a folha recebe
 * mu = d(folha, x) e x vira seu filho externo.
 * Retorna false se faltar memoria. */
bool vp_inserir(VPTree *t, const void *ponto);

/* Remove um ponto x (o no cujo ponto esta a distancia 0 de x).
 * O no e localizado por busca exata e sua subarvore e reconstruida com os
 * descendentes. Se 'removido' nao for NULL, recebe o ponteiro do ponto
 * que estava armazenado (para o chamador liberar, se quiser).
 * Retorna false se o ponto nao existir ou se faltar memoria. */
bool vp_remover(VPTree *t, const void *ponto, const void **removido);

/* ---------------- Buscas ---------------- */
/* Em todas as buscas, se 'calculos' nao for NULL ele recebe o numero de
 * distancias calculadas, para comparar com a busca exaustiva (n). */

/* Busca exata (raio 0): percorre um unico caminho da raiz ate o ponto.
 * Retorna o ponto armazenado ou NULL se nao existir. */
const void *vp_buscar_exato(const VPTree *t, const void *ponto, size_t *calculos);

/* Vizinho mais proximo de q. Retorna NULL se a arvore estiver vazia.
 * Se 'distancia' nao for NULL, recebe d(q, vizinho). */
const void *vp_vizinho_mais_proximo(const VPTree *t, const void *q,
                                    double *distancia, size_t *calculos);

/* Os k vizinhos mais proximos de q, em ordem crescente de distancia.
 * 'vizinhos' e 'distancias' (pode ser NULL) devem ter espaco para k itens.
 * Retorna quantos foram encontrados (min(k, tamanho)), ou 0 sem memoria. */
size_t vp_k_vizinhos(const VPTree *t, const void *q, size_t k,
                     const void **vizinhos, double *distancias, size_t *calculos);

/* Busca por faixa: todos os pontos x com d(q, x) <= raio.
 * Grava no maximo 'capacidade' pontos em 'saida' (pode ser NULL) e retorna
 * o total encontrado (que pode ser maior que a capacidade). */
size_t vp_buscar_faixa(const VPTree *t, const void *q, double raio,
                       const void **saida, size_t capacidade, size_t *calculos);

/* ---------------- Consultas ---------------- */

bool vp_vazia(const VPTree *t);
size_t vp_tamanho(const VPTree *t);

/* Altura da arvore (vazia = 0, um no = 1). Recursiva. */
size_t vp_altura(const VPTree *t);

/* Verifica a propriedade de particao por bola em todos os nos
 * (interno: d < mu, externo: d >= mu) e o tamanho. O(n * altura). */
bool vp_valida(const VPTree *t);

/* Imprime a arvore em formato hierarquico (int = interno, ext = externo). */
void vp_imprimir(const VPTree *t, FILE *saida);

#ifdef __cplusplus
}
#endif

#endif /* VP_TREE_H */
