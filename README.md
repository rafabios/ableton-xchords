# xchords

Nome do produto: **Xchords - Harmony**. Fabricante VST3: **Vemcompy Labs**.
Esses são os metadados enviados à DAW; a exibição em árvore depende do agrupamento por fabricante no navegador do host. Os identificadores internos do plugin e o formato de estado foram preservados nesta mudança de marca.

Explorador de acordes e escalas para Windows x64, em C++17/JUCE. Projeto independente do xslice. Saídas: aplicativo standalone e VST3.

## Downloads e publicação

- Site: https://xchords.vemcompy.tec.br
- Último build de desenvolvimento: https://github.com/rafabios/ableton-xchords/releases/tag/latest
- Releases: https://github.com/rafabios/ableton-xchords/releases
- CI: https://github.com/rafabios/ableton-xchords/actions

O workflow `Build Windows` compila em Release x64 no Windows Server 2022, executa os testes e empacota VST3 e standalone separadamente, com checksums SHA-256. Pushes de código na `main` atualizam a pré-release `latest`; tags `v*` publicam releases versionadas. Pull requests apenas compilam, testam e geram artefatos. Também é possível disparar o workflow manualmente. Não há certificado de assinatura de código configurado.

Para empacotar localmente após compilar: `./scripts/Package.ps1 -Configuration Release`. Os ZIPs ficam em `dist/` e não entram no Git.

O workflow `Deploy website` publica `site/` no GitHub Pages. Nas configurações do repositório, a fonte do Pages deve ser **GitHub Actions**, com domínio customizado `xchords.vemcompy.tec.br`. No provedor DNS, crie um CNAME `xchords` apontando para `rafabios.github.io` (sem caminho de repositório). Ative HTTPS quando o GitHub concluir a emissão do certificado. Documentação: https://docs.github.com/en/pages/configuring-a-custom-domain-for-your-github-pages-site/managing-a-custom-domain-for-your-github-pages-site

Os links de download no site apontam para assets públicos da release `latest`; a API é usada apenas para exibir a data. Se a consulta da API falhar, os links continuam disponíveis. A primeira execução precisa terminar para disponibilizar os pacotes.

## Interface redimensionável

- Arraste a alça do canto inferior direito para redimensionar. Tamanho inicial: 1180 × 800; limites: 1020 × 720 a 1800 × 1200.
- Painéis de harmonia, audição, notas e Camelot ajustam suas larguras; teclado ocupa a base da janela. A área das notas cresce com a altura disponível.
- O tamanho é preservado ao reabrir o editor e faz parte do estado salvo na sessão da DAW.
- Tema próprio (`src/Theme.h`) com painéis escuros, controles arredondados, contraste de foco e destaque verde para reprodução e notas selecionadas. Idiomas PT/EN preservados.
- Testes de layout em 1020 × 720, 1180 × 800 e 1600 × 1000, incluindo cliques após redimensionamento e restauração das dimensões. O redimensionamento dentro do Ableton ainda requer validação no host.

## Versão 0.3 — Biblioteca, audição e progressões

- **Presets e favoritos:** abra a biblioteca, dê um nome e salve a cena completa, incluindo notas, timbre e progressão. Marque favoritos e filtre a lista. Arquivos locais em `%APPDATA%/Vemcompy Labs/Xchords/Presets`. Carregar preserva idioma, convenção de oitava e tamanho da janela atuais.
- **Desfazer/refazer:** histórico de até 128 alterações enquanto o editor estiver aberto. O histórico não é salvo na sessão; a cena musical é.
- **Audição:** três timbres (teclas suaves, pad e senoide), com destaque âmbar nas teclas que estão soando. Verde indica a seleção.
- **BPM da DAW:** opção para acompanhar o tempo informado pelo host, inclusive mudanças durante a prévia. Sem host ou sem BPM disponível, usa o valor manual. A sincronização é de tempo; iniciar/parar o transporte da DAW não inicia/paralisa a prévia.
- **Progressões:** adicione a seleção atual, reordene ou remova passos e escolha a duração individual entre 0,25 e 32 tempos. Até 128 passos. A reprodução da sequência usa notas em bloco e permite loop.
- Para alterar um passo, selecione-o e use a edição de notas. Ajuste o teclado, volte à progressão e substitua o passo selecionado; sua duração é mantida.
- **Exportar MIDI** no painel de progressão gera a sequência inteira em um arquivo, com um ciclo e as durações individuais. Os botões principais de salvar/arrastar continuam exportando a seleção do teclado.

Os testes usam um host simulado para validar BPM e mudanças de tempo. A integração com o Ableton real continua pendente.

## Versão 0.2 — Camelot e edição de notas

- Roda Camelot clicável com 24 tonalidades: anel A menor, anel B maior. Seleciona a fundamental e a fórmula maior/menor no modo atual (acorde ou escala).
- Referência: https://mixedinkey.com/camelot-wheel/. 8A = Lá menor; 8B = Dó maior; 1A = Sol# / Lá♭ menor.
- Teclas clicáveis: uma nota selecionada é removida; uma tecla livre é incluída. Cada tecla representa uma altura MIDI específica; mudar Dó4 não altera Dó5.
- A combinação passa a ser **Personalizada**. A roda deixa de afirmar um código para a combinação editada e mostra apenas a referência de origem. Outras fórmulas também não recebem um código maior/menor arbitrário.
- **Loop** repete a prévia até Parar; desativá-lo deixa terminar o ciclo atual. Enquanto toca, editar notas, tempo, duração ou intensidade reinicia a prévia com a nova seleção. Marcar Loop sozinho não inicia o áudio: clique Ouvir.
- **Restaurar original** recupera a seleção da fórmula. Escolher outra fundamental/fórmula, mudar modo, sortear ou clicar na roda inicia uma nova seleção. Mudar oitava transpõe as notas personalizadas.
- Exportação e arraste geram um ciclo com as notas editadas; Loop controla a audição, não cria um MIDI infinito. Uma seleção vazia desativa Ouvir e exportação.
- Notas editadas e opção Loop ficam salvas na sessão, sem reprodução automática ao restaurar.

## Recursos iniciais

- 15 fórmulas de acordes e 14 escalas, calculadas localmente, sem depender do site de referência.
- Fundamental, inversão, oitava e execução em bloco, ascendente, descendente ou ida e volta.
- Teclado de três oitavas com notas selecionadas e lista de nomes equivalentes: letras, solfejo, sustenidos, bemóis, oitava e número MIDI.
- Exemplo: `A#1 / Bb1 · Lá#1 / Si♭1 · MIDI 34`. A convenção é selecionável: MIDI 60 = C4/Dó4 ou C3/Dó3. Ela muda os rótulos, nunca a altura.
- Interface PT-BR/English; nomes de notas em ambos os sistemas permanecem visíveis.
- Sorteio dentro do modo atual, opção de manter a fundamental e desfazer o último sorteio.
- Prévia com três timbres sintetizados, intensidade e BPM; recebe também notas MIDI da DAW.
- Exportação `.mid` com 960 ticks por semínima, tempo, compasso 4/4 e note-on/note-off. Arraste usa um arquivo temporário em `%TEMP%/xchords`.
- Estado musical e idioma salvos no projeto da DAW. A reprodução não reinicia automaticamente.

Os nomes exibidos são equivalências de altura, não uma grafia harmônica contextual completa. Por exemplo, certos acordes exigem Mi# ou duplos acidentes na escrita teórica; essa grafia fica para uma próxima etapa. A escala menor melódica usa a forma ascendente também quando tocada ao contrário.

## Compilar

Instale Visual Studio 2022 Build Tools com C++, Windows SDK e CMake. Execute:

```powershell
./Build.ps1
```

Para reutilizar um checkout local do JUCE:

```powershell
./Build.ps1 -JuceSource 'I:/Users/Rafael/Documents/git/vemcompy/xslice/.local/deps/juce-src'
```

Sem esse argumento, o CMake baixa JUCE 9.0.4 de um commit fixo, validado por SHA-256. O licenciamento do JUCE precisa ser considerado antes da distribuição do produto.

Saídas atuais em `build/XChords_artefacts/Debug/Standalone/Xchords - Harmony.exe` e `build/XChords_artefacts/Debug/VST3/Xchords - Harmony.vst3`. O build não instala o plugin automaticamente. Artefatos antigos com o nome XChords podem continuar na pasta de build; use somente o bundle com o novo nome ao instalar para evitar duplicatas com a mesma identidade.

## Experimentar

1. Abra o standalone ou carregue o VST3 na DAW.
2. Escolha Acordes/Escalas, fundamental e fórmula.
3. Confira os nomes das notas e escolha oitava, inversão e execução.
4. Clique em Ouvir. Parar interrompe a prévia.
5. Use Salvar MIDI ou pressione e arraste o botão Arrastar MIDI para uma pista MIDI.

O arraste e o carregamento do VST3 precisam de validação manual na DAW de destino. O plugin gera arquivos MIDI; não edita diretamente clips existentes e não fornece saída MIDI em tempo real nesta versão. A duração indica o comprimento de cada nota em tempos, não o comprimento total da escala.

## Arquitetura e próximas etapas

`src/Theory.*` contém fórmulas, nomes, inversões, Camelot e sequenciamento, independente do JUCE. `src/MusicalState.h` modela a seleção original/editada usada tanto na prévia quanto na exportação. `src/CamelotWheel.h` contém a roda interativa. `src/MidiExport.h` exporta eventos para Standard MIDI File. `src/Plugin.cpp` contém integração com host, áudio e interface.

1. Intervalos e grafia contextual correta (incluindo acidentes duplos).
2. Transições entre posições e condução de vozes nas progressões.
3. Sorteio de acordes restrito a uma escala, padrões rítmicos e voicings abertos.
4. Perfis de instrumento: teclado, guitarra, baixo e ukulele. Cada perfil determina extensão, afinação, posições e regras de execução; todos usam o mesmo núcleo musical e saída MIDI.
5. Integrações específicas com DAWs para trabalhar com clips existentes, se necessário.

Referência de produto: https://chords.jpglomot.com/. Nenhum código, áudio ou banco de dados foi copiado do site.

## Validação em 28/10/2026

Build Debug Windows x64 concluído para standalone e VST3. CTest: 2/2 suites passaram, cobrindo nomes/oitavas, fórmulas, inversões, sequências, extensão MIDI, exportação com leitura de volta, áudio, restauração de estado e criação da interface. Imagens PT/EN inspecionadas em `build/test-artifacts/`. Carregamento e drag-and-drop no Ableton ainda não validados.

A versão 0.2 acrescenta testes para as 24 correspondências Camelot, cliques em teclas brancas/pretas e setores da roda, troca de nota durante a reprodução, exportação das notas personalizadas, loop além do primeiro ciclo, desligamento de loop, Parar/cancelamento de prévia pendente, restauração de notas e seleção vazia.

A versão 0.3 acrescenta testes de presets/favoritos persistentes, desfazer/refazer, edição e restauração de progressões, MIDI com duração total conferida, timbres distintos, destaque das teclas e BPM de host simulado com fallback manual.
