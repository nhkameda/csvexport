# Prompt: CSVExport Web App para Kameda ERP

Cole este prompt no Claude Code dentro da pasta `/Users/mac/KAMEDA/csvexport`

---

## CONTEXTO

Você vai recriar a funcionalidade da DLL `csvexport.dll` (plugin do VisualPlace para
exportação de Pick & Place CSV) como uma aplicação web standalone, que futuramente
será integrada ao Kameda ERP localizado em `/Users/mac/KAMEDA/erp-kameda`.

O ERP usa o seguinte stack: Node.js 20 + Fastify 4 + TypeScript 5 (backend),
React 18 + Vite 5 + TailwindCSS 3 (frontend), PostgreSQL 16, Drizzle ORM.

## FASE 1 — LEITURA E ANÁLISE

Antes de escrever qualquer código, leia e entenda:

1. Código fonte da DLL: `assets/dll/csvexport.c` e `assets/dll/csvexport.h`
2. Template de exemplo: `assets/template/csvexport.ini`
3. CSV de entrada: `assets/csv/PCB_A_ori.csv` e `assets/csv/PCB_B_ori.csv`
4. CSVs exportados:
   - `assets/csv/PCB_A_top.csv` e `assets/csv/PCB_A_bottom.csv`
   - `assets/csv/PCB_B_top.csv` e `assets/csv/PCB_B_bottom.csv`

Entenda exatamente o que a DLL faz: lê um CSV de Pick & Place do Altium Designer,
aplica um template de configuração (colunas, separador, unidade, ângulo, lado, etc.)
e gera os arquivos exportados separados por lado (Top/Bottom).

## FASE 2 — LÓGICA DE NEGÓCIO A IMPLEMENTAR

A lógica central do csvexport.c que deve ser reimplementada em TypeScript/JavaScript:

### Parser do CSV de entrada (formato Altium Designer):
- Ignora linhas de cabeçalho até encontrar a linha com "Designator"
- Detecta separador automaticamente (vírgula ou ponto-e-vírgula)
- Campos: Designator, Value, X, Y, Rotation, Side (Top/Bottom), Mount (Y/N), ProductNr, Package, etc.

### Lógica de exportação (baseada em `vp_WriteCentroid`):
- **Colunas configuráveis** (até 11): Sequence, Designator, Value, Package,
  ProductNr, X(mm), Y(mm), Rotation, Side, Stage, Mount Y/N
- **Separador**: vírgula, ponto-e-vírgula ou tab
- **Unidade**: mm ou inch (1 inch = 25.4 mm)
- **Lado**: Top+Bottom separados, Top+Bottom combinado, Só Top, Só Bottom
- **Ângulo**: range 0-360 ou -180-180; direção horária ou anti-horária
- **Enquote**: envolver campos em aspas duplas
- **Invert Y**: inverter o eixo Y
- **Zero-orientation**: ajuste de rotação base para:
  - Componentes passivos não-polarizados (R, C, L, X): padrão "Left"
  - Componentes polarizados/diodos: padrão "Left"
  - Componentes gerais (>2 pinos): padrão "Up"
- **Include no-mount**: incluir ou não componentes com Mount=N
- **CSV header**: incluir comentários de configuração no topo do arquivo

### Sistema de Templates (baseado em `csvexport.ini`):
- Formato INI com seções `[cpl:NOME_TEMPLATE]`
- Cada template armazena todas as configurações de exportação
- Deve carregar o template `YAMAHA` do `csvexport.ini` como template inicial de exemplo
- Permitir criar, salvar e carregar templates no browser (localStorage ou arquivo .ini)
- Estrutura do template:
  ```
  centroid1..11 = índice da coluna (0=none, 1=seq, 2=desig, 3=value, 4=pkg, 5=prodnr, 6=X, 7=Y, 8=rot, 9=side, 10=stage, 11=mount)
  zero1 = orientação componentes passivos (0=Up, 1=Left, 2=Down, 3=Right)
  zero2 = orientação diodos/polarizados
  zero3 = orientação misc (>2 pinos)
  side = 0=Top+Bot sep, 1=Top+Bot combined, 2=Top only, 3=Bottom only
  unit = 0=mm, 1=inch
  enquote-fields = 0/1
  align-fiducial = 0/1
  invert-y = 0/1
  angle-range = 0=(0-360), 1=(-180 a 180)
  angle-clockwise = 0/1
  separator = 0=comma, 1=semicolon, 2=tab
  ```

## FASE 3 — ESTRUTURA DO PROJETO A CRIAR

Crie a estrutura em `/Users/mac/KAMEDA/csvexport/webapp/`:

```
webapp/
├── package.json          # Node.js + TypeScript
├── tsconfig.json
├── src/
│   ├── server.ts         # Fastify server (porta 3333)
│   ├── routes/
│   │   ├── export.ts     # POST /api/export — processa e retorna CSVs
│   │   └── templates.ts  # GET/POST /api/templates — gestão de templates
│   ├── core/
│   │   ├── parser.ts     # Parser do CSV de entrada (Altium format)
│   │   ├── exporter.ts   # Lógica de exportação (reimplementação de vp_WriteCentroid)
│   │   └── template.ts   # Leitura/escrita de templates .ini
│   └── types.ts          # Tipos TypeScript (PartInfo, ExportConfig, Template)
├── frontend/
│   ├── index.html
│   ├── vite.config.ts
│   ├── src/
│   │   ├── main.tsx
│   │   ├── App.tsx
│   │   └── components/
│   │       ├── FileUpload.tsx     # Upload do CSV de entrada
│   │       ├── ColumnMapper.tsx   # Seleção das 11 colunas
│   │       ├── ExportOptions.tsx  # Configurações de exportação
│   │       ├── TemplateManager.tsx # Carregar/salvar templates
│   │       └── ExportResult.tsx   # Download dos arquivos gerados
│   └── package.json
└── assets/               # Copiar os arquivos de exemplo aqui
    ├── csv/
    └── template/
```

## FASE 4 — INTERFACE WEB (UI)

A interface deve ser **fiel à DLL original**, mas em formato web moderno.
Use TailwindCSS. Layout em uma única página dividida em seções:

### Seção 1 — Upload do arquivo
- Área de drag-and-drop para o CSV de entrada
- Preview das primeiras linhas do CSV carregado
- Detecção automática do separador e colunas disponíveis

### Seção 2 — Template
- Dropdown com templates salvos (carrega "YAMAHA" do csvexport.ini como padrão)
- Botão "Novo Template" (abre modal para nomear)
- Botão "Salvar Template" (salva configuração atual)
- Botão "Exportar Template (.ini)" para download do arquivo

### Seção 3 — Mapeamento de Colunas (11 colunas)
- 11 dropdowns em grade (como a DLL original)
- Opções: (none), Sequence, Designator, Value, Package, Product Nr, X, Y, Rotation, Side, Stage, Mount Y/N
- Valor padrão do template YAMAHA pré-carregado:
  Col1=Designator, Col2=Value, Col3=X, Col4=Y, Col5=Rotation, Col6=Mount, Col7=ProductNr

### Seção 4 — Opções de Exportação
- **Lado**: Top+Bottom separados | Top+Bottom combinado | Só Top | Só Bottom
- **Separador**: Vírgula | Ponto-e-vírgula | Tab
- **Unidade**: mm | inch
- **Ângulo**: range 0-360 / -180-180; Horário / Anti-horário
- **Zero-orientation** (3 dropdowns: passivos, diodos, misc): Up/Left/Down/Right
- **Checkboxes**: Enquote fields | Invert Y | Align to fiducial | Include fiducials | Include no-mount | CSV header

### Seção 5 — Exportar
- Botão "Exportar" (grande, azul)
- Resultado: lista de arquivos gerados com botão de download para cada um
- Preview do conteúdo de cada arquivo exportado

## FASE 5 — VALIDAÇÃO

Após implementar, valide que a saída é idêntica aos exemplos:

```bash
# Processar PCB_A_ori.csv com template YAMAHA deve gerar:
# - PCB_A_top.csv idêntico ao exemplo em assets/csv/PCB_A_top.csv
# - PCB_A_bottom.csv idêntico ao exemplo em assets/csv/PCB_A_bottom.csv
# Idem para PCB_B
```

Crie um script de teste automático `test/validate.ts` que:
1. Carrega PCB_A_ori.csv + template YAMAHA
2. Executa a exportação
3. Compara linha a linha com os arquivos de referência
4. Reporta qualquer diferença

## FASE 6 — SCRIPTS DE EXECUÇÃO

Crie scripts simples para rodar localmente:

```json
// package.json raiz (webapp/)
{
  "scripts": {
    "dev": "concurrently \"npm run dev:server\" \"npm run dev:frontend\"",
    "dev:server": "ts-node-dev src/server.ts",
    "dev:frontend": "cd frontend && npm run dev",
    "test": "ts-node test/validate.ts",
    "build": "tsc && cd frontend && npm run build"
  }
}
```

Ao final rode `npm run dev` e confirme que:
- Backend está rodando em http://localhost:3333
- Frontend está rodando em http://localhost:5173
- Upload de PCB_A_ori.csv + template YAMAHA + clique em Exportar gera os arquivos corretos

## NOTAS IMPORTANTES

- Mantenha o código preparado para integração futura com o ERP Kameda
  (mesmo stack: Fastify + TypeScript + React + TailwindCSS)
- O módulo `core/exporter.ts` deve ser 100% independente do servidor HTTP
  (função pura que recebe dados e retorna CSVs) — facilitará a importação no ERP
- Documente as funções principais com JSDoc
- Use TypeScript strict mode
