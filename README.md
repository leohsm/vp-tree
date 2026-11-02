# VP-Tree em C

**UTFPR - Câmpus Medianeira | Ciência da Computação | Árvores e Grafos | 2026.2**

**Equipe:** Erik Mazzuco, Letícia Moro, Leonardo Herrero

Biblioteca em C de uma **VP-Tree** (*Vantage-Point Tree*), com um programa de exemplo e testes automatizados.
Trabalho do Seminário 3 – Árvores Espaciais e Geométricas da disciplina **Árvores e Grafos** (UTFPR – Câmpus Medianeira, 2026.2).

A VP-Tree (Yianilos, 1993; Uhlmann, 1991) é uma árvore binária para **busca por similaridade em espaços métricos**.
Cada nó guarda um **ponto de vantagem** (vp) e um raio **μ**: os elementos com `d(vp, x) < μ` ficam na subárvore
**interna** e os demais (`d(vp, x) ≥ μ`) na **externa**. Na construção, μ é a mediana das distâncias, o que deixa a
árvore balanceada. Nas buscas, a **desigualdade triangular** permite descartar subárvores inteiras sem calcular
nenhuma distância dentro delas.

A biblioteca não conhece o tipo dos pontos: eles são ponteiros (`const void *`) comparados apenas por uma função de
distância fornecida pelo usuário. Por isso a mesma árvore funciona para pontos no plano, vetores de qualquer dimensão
ou palavras com distância de edição.

## Estrutura

```
include/vp_tree.h     interface pública da biblioteca
src/vp_tree.c         implementação
examples/exemplo.c    programa de exemplo (demonstração guiada + modo interativo)
tests/teste.c         testes automatizados
Makefile              compilação com make
compilar.bat          compilação no Windows sem make
docs/                 relatório (vp_tree_doc.pdf e vp_tree_doc.docx)
```

O relatório completo está em [`docs/vp_tree_doc.pdf`](docs/vp_tree_doc.pdf) (também em [.docx](docs/vp_tree_doc.docx)).

## Como rodar

Primeiro baixe o projeto:

```sh
git clone https://github.com/leohsm/vp-tree.git
cd vp-tree
```

Depois escolha uma das opções abaixo: compilar direto em C (opção 1) ou usar o Docker sem instalar
compilador (opção 2).

### Opção 1: compilando em C (gcc)

#### Windows

1. Instale o gcc (só na primeira vez) pelo PowerShell:

   ```powershell
   winget install BrechtSanders.WinLibs.POSIX.UCRT
   ```

   Feche e abra o terminal de novo e confira com `gcc --version`.

2. Compile e rode na pasta do projeto:

   ```powershell
   .\compilar.bat         # gera libvptree.a, exemplo.exe e teste_vp.exe
   .\exemplo.exe          # demonstração guiada
   .\exemplo.exe -i       # menu interativo
   .\teste_vp.exe         # testes automatizados
   ```

#### Linux / macOS

Instale o gcc e o make, se ainda não tiver (`sudo apt install build-essential` no Ubuntu/Debian,
`xcode-select --install` no macOS), e rode:

```sh
make             # gera libvptree.a e o programa exemplo
./exemplo        # demonstração guiada
./exemplo -i     # menu interativo
make teste       # compila e roda os testes
make clean       # apaga os arquivos gerados
```

#### Sem make nem .bat (qualquer sistema)

```sh
gcc -std=c99 -Wall -Iinclude -c src/vp_tree.c -o vp_tree.o
ar rcs libvptree.a vp_tree.o
gcc -std=c99 -Wall -Iinclude examples/exemplo.c -L. -lvptree -lm -o exemplo
gcc -std=c99 -Wall -Iinclude tests/teste.c -L. -lvptree -lm -o teste_vp
```

### Opção 2: usando Docker

Não precisa instalar compilador, só o [Docker](https://www.docker.com/products/docker-desktop/).
No Windows, **abra o Docker Desktop antes** de rodar os comandos. A compilação acontece dentro do
container, a partir de uma cópia do projeto, então nenhum arquivo é criado na sua pasta.

**Windows (PowerShell):**

```powershell
# demonstração guiada
docker run --rm -v "${PWD}:/src:ro" gcc:14 sh -c "cp -r /src /w && cd /w && make && ./exemplo"

# menu interativo (o -it permite digitar no programa)
docker run --rm -it -v "${PWD}:/src:ro" gcc:14 sh -c "cp -r /src /w && cd /w && make && ./exemplo -i"

# testes
docker run --rm -v "${PWD}:/src:ro" gcc:14 sh -c "cp -r /src /w && cd /w && make teste"
```

**Linux / macOS / Git Bash:**

```sh
# demonstração guiada
docker run --rm -v "$(pwd):/src:ro" gcc:14 sh -c "cp -r /src /w && cd /w && make && ./exemplo"

# menu interativo
docker run --rm -it -v "$(pwd):/src:ro" gcc:14 sh -c "cp -r /src /w && cd /w && make && ./exemplo -i"

# testes
docker run --rm -v "$(pwd):/src:ro" gcc:14 sh -c "cp -r /src /w && cd /w && make teste"
```

No Git Bash do Windows, coloque `MSYS_NO_PATHCONV=1` antes de `docker` para o caminho não ser convertido.
Na primeira execução o Docker baixa a imagem `gcc:14` (mais de 1 GB), o que pode demorar alguns minutos.

## Programa de exemplo

```sh
./exemplo       # demonstração guiada
./exemplo -i    # menu interativo (inserir, remover, buscar, k vizinhos, faixa, imprimir...)
```

A demonstração reproduz os exemplos do relatório, com o **rastro passo a passo** de cada operação:

1. Construção da árvore com os pontos A(5,5), B(6,6), C(3,4), D(5,8), E(9,2), F(1,9), G(10,7) (Figura 2)
2. Inserção de X(7,5) (Figura 3)
3. Remoção de B, com reconstrução da subárvore (Figura 4)
4. Vizinho mais próximo de Q(4,4), com poda pela desigualdade triangular (Figura 6)
5. Busca exata, k vizinhos mais próximos e busca por faixa
6. Espaço métrico sem coordenadas: corretor ortográfico com distância de Levenshtein
7. Desempenho em 2 a 32 dimensões (maldição da dimensionalidade)

Exemplo de saída (construção e busca do vizinho mais próximo de Q):

```
A(5,5)  mu=4.00
|-- int: B(6,6)  mu=2.92
|   |-- int: D(5,8)
|   `-- ext: C(3,4)
`-- ext: E(9,2)  mu=7.86
    |-- int: G(10,7)
    `-- ext: F(1,9)
  [tamanho=7  altura=3  valida=sim]

Busca dos 1 vizinho(s) mais proximo(s) de Q(4,4)
  (1) visita A(5,5): d = 1.41  ->  tau = 1.41
    (2) visita B(6,6): d = 2.83
      (3) visita D(5,8): d = 4.12
      (4) visita C(3,4): d = 1.00  ->  tau = 1.00
    poda: subarvore externa de A(5,5) descartada (d + tau = 1.41 + 1.00 = 2.41 < mu = 4.00)

Resultado: C(3,4) a distancia 1.00, com 4 calculos de distancia (busca exaustiva: 7).
```

Desempenho com 50 mil pontos aleatórios (cálculos de distância por consulta de vizinho mais próximo):

```
   D | altura | construcao (s) | calculos/consulta | % da base
-----+--------+----------------+-------------------+----------
   2 |     16 |          0.023 |                24 |     0.0%
   4 |     16 |          0.025 |               101 |     0.2%
   8 |     16 |          0.029 |              2310 |     4.6%
  16 |     16 |          0.039 |             38555 |    77.1%
  32 |     16 |          0.064 |             49999 |   100.0%
```

## API

```c
#include "vp_tree.h"

double dist(const void *a, const void *b);         /* a sua métrica */

VPTree *t = vp_criar(dist, imprimir, VP_ESCOLHA_DISPERSAO);

vp_construir(t, pontos, n);                        /* árvore balanceada pela mediana */
vp_inserir(t, &novo);                              /* descida guiada por mu          */
vp_remover(t, &alvo, &removido);                   /* reconstrói a subárvore do nó   */

double d;
size_t calculos;
const void *viz = vp_vizinho_mais_proximo(t, &q, &d, &calculos);

const void *k[5];
vp_k_vizinhos(t, &q, 5, k, NULL, NULL);            /* 5 mais próximos, em ordem      */
size_t total = vp_buscar_faixa(t, &q, 2.5, res, cap, NULL);   /* d(q, x) <= 2.5      */

vp_definir_rastro(t, stdout);                      /* descreve cada passo            */
vp_imprimir(t, stdout);
vp_destruir(t);                                    /* não libera os pontos           */
```

| Função | Descrição | Complexidade |
|---|---|---|
| `vp_criar` / `vp_destruir` / `vp_limpar` | cria, libera, esvazia (os pontos são do usuário) | O(1) / O(n) / O(n) |
| `vp_construir(t, pontos, n)` | constrói a árvore balanceada (μ = mediana, *quickselect*, empates distribuídos) | O(n log n) esperado |
| `vp_inserir(t, x)` | desce guiada por μ; a folha alcançada recebe μ = d e x vira filho externo | O(log n), pior O(n) |
| `vp_remover(t, x, &removido)` | localiza o nó e reconstrói sua subárvore com os descendentes | O(log n + m log m) |
| `vp_buscar_exato(t, x, &calc)` | busca com raio 0: um único caminho | O(log n), pior O(n) |
| `vp_vizinho_mais_proximo(t, q, &d, &calc)` | vizinho mais próximo, com poda | O(log n) esperado* |
| `vp_k_vizinhos(t, q, k, viz, dist, &calc)` | k mais próximos em ordem crescente (max-heap de tamanho k) | O(log n) esperado* |
| `vp_buscar_faixa(t, q, r, saida, cap, &calc)` | todos os pontos com d(q, x) ≤ r | O(log n + k) esperado* |
| `vp_definir_rastro(t, arquivo)` | liga/desliga a descrição passo a passo das operações | – |
| `vp_imprimir`, `vp_valida`, `vp_altura`, `vp_tamanho` | depuração e visualização | O(n) / O(n·h) / O(n) / O(1) |

\* Para dados de baixa dimensão intrínseca e raios pequenos; no pior caso (ou em dimensão muito alta) as buscas são O(n).
O parâmetro `calc` (pode ser `NULL`) recebe o número de distâncias calculadas, para comparar com a busca exaustiva.

Escolha do ponto de vantagem (`VPEscolha`):

- `VP_ESCOLHA_PRIMEIRO`: primeiro elemento da lista (didático, reproduz os exemplos do relatório);
- `VP_ESCOLHA_ALEATORIA`: elemento sorteado;
- `VP_ESCOLHA_DISPERSAO`: heurística de Yianilos, que escolhe o candidato com maior variância das distâncias.

## Operação fundamental: construção

```c
static VPNo *construir_rec(Construtor *c, Item *itens, size_t n, int nivel)
{
    ...
    escolher_vp(c, itens, n);                     /* ponto de vantagem em itens[0] */
    no = novo_no(itens[0].p);
    if (m == 0) return no;                        /* folha */

    for (i = 0; i < m; i++)                       /* distâncias até o vp */
        resto[i].d = c->ds[i] = t->dist(no->ponto, resto[i].p);
    no->mu = mediana(c->ds, m);                   /* quickselect, O(m) */

    /* partição estável: d < mu -> S_in, d > mu -> S_out;
       empates (d == mu) completam S_in até floor(m/2) elementos */
    ...
    no->interno = construir_rec(c, resto, nin, nivel + 1);
    no->externo = construir_rec(c, resto + nin, m - nin, nivel + 1);
    return no;
}
```

## Testes

`make teste` executa mais de 311 mil verificações:

- exemplos do relatório (forma exata da árvore e valores de μ após construção, inserção e remoção; busca de Q(4,4)
  com exatamente 4 cálculos de distância);
- casos limite (árvore vazia, 1 e 2 pontos, k maior que o tamanho, capacidade menor que o resultado);
- 6 mil operações aleatórias para cada estratégia de escolha do vp, comparando vizinho mais próximo, k vizinhos e
  busca por faixa com uma busca exaustiva, e validando a propriedade de partição de todos os nós;
- palavras com distância de Levenshtein comparadas com busca exaustiva;
- empates na fronteira: 2000 pontos idênticos (altura 11, e não 2000), 12 pontos à mesma distância do vp e uma
  grade 12×12 com raios inteiros exatamente sobre as fronteiras, com inserções e remoções;
- árvore degenerada com 20 mil nós (inserções em linha reta) que volta a ficar balanceada ao remover a raiz.

Os testes também passam com `-fsanitize=address,undefined`.

## Observações

- A árvore não copia nem libera os pontos: `vp_remover` devolve o ponteiro removido para o chamador liberar, se quiser.
- **Empates na mediana:** se várias distâncias forem iguais a μ, uma regra rígida (`d < μ` interno, `d ≥ μ` externo)
  mandaria todas para o mesmo lado e degeneraria a árvore (ex.: pontos duplicados ou distâncias inteiras). Na
  construção, os elementos com `d == μ` completam a subárvore interna até ⌊m/2⌋ elementos, então cada nó divide
  seus elementos em ⌊m/2⌋ e ⌈m/2⌉ e a altura é sempre ⌈log₂(n + 1)⌉. Por isso, a subárvore interna contém
  `d ≤ μ` e a externa `d ≥ μ`; a busca exata verifica os dois lados apenas em um empate exato `d == μ`.
- Um ponto é identificado pela distância 0 (`d(x, y) = 0` se e somente se `x = y` em uma métrica). Com `double`,
  isso pressupõe que a função de distância devolva exatamente 0 para pontos iguais (vale para a euclidiana com as
  mesmas coordenadas e para métricas inteiras); funções com erro de arredondamento podem precisar de tolerância.
- A função de distância precisa ser uma **métrica** (em especial, respeitar a desigualdade triangular). A
  similaridade de cosseno, por exemplo, não é; use a distância angular ou a euclidiana entre vetores normalizados.
- `vp_construir`, `vp_inserir`, `vp_buscar_exato`, `vp_remover` e `vp_destruir` não estouram a pilha em árvores
  degeneradas. As buscas por vizinhos e por faixa, `vp_imprimir`, `vp_valida` e `vp_altura` são recursivas, com
  profundidade igual à altura da árvore; depois de muitas inserções, use `vp_construir` para rebalancear.

## Referências

- YIANILOS, P. N. *Data structures and algorithms for nearest neighbor search in general metric spaces*. SODA, 1993.
- UHLMANN, J. K. *Satisfying general proximity/similarity queries with metric trees*. Information Processing Letters, 1991.
- CHÁVEZ, E. et al. *Searching in metric spaces*. ACM Computing Surveys, 2001.
- FU, A. W. et al. *Dynamic vp-tree indexing for n-nearest neighbor search given pair-wise distances*. The VLDB Journal, 2000.
