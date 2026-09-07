import SwiftUI

@main
struct ChessApp: App {
    @StateObject private var game = ChessGame()
    @Environment(\.scenePhase) private var scenePhase

    var body: some Scene {
        WindowGroup {
            ChessView(game: game)
                .onChange(of: scenePhase) { phase in
                    if phase != .active { game.save() }
                }
        }
    }
}

enum ChessText {
    static func choose(_ english: String, _ chinese: String) -> String {
        Locale.preferredLanguages.first?.hasPrefix("zh") == true ? chinese : english
    }
}

@MainActor
final class ChessGame: ObservableObject {
    @Published private(set) var cells = Array(repeating: 0, count: 225)
    @Published private(set) var moves: [Int] = []
    @Published private(set) var hints: [Int] = []
    @Published private(set) var busy = false
    @Published private(set) var showingThinking = false
    @Published private(set) var loadingHints = false
    @Published private(set) var winner = 0
    @Published private(set) var player = 1
    @Published private(set) var forbidden = false
    @Published var selected: Int?
    @Published var message: String?
    @Published var difficulty = 3 { // Four stars; keep the legacy persisted mapping.
        didSet { engine.difficulty = difficulty; engine.save() }
    }
    private let engine: ChessEngine
    private var thinkingDelay: Task<Void, Never>?

    init() {
        let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        engine = ChessEngine(save: directory.appendingPathComponent("chess-game.plist"))
        difficulty = engine.difficulty
        refresh()
        // Resume an interrupted computer turn from the saved human move.
        if engine.computerTurn { analyze() }
    }

    var finished: Bool { winner != 0 || moves.count == 225 }
    var hasUndoableMoves: Bool { moves.count > (player == -1 ? 1 : 0) }
    var canUndo: Bool { !busy && hasUndoableMoves }
    var showingHints: Bool { !hints.isEmpty }
    var showingProgress: Bool { loadingHints || showingThinking }
    var status: String {
        if loadingHints { return ChessText.choose("Finding a possible continuation…", "正在分析参考走法…") }
        if showingThinking { return ChessText.choose("Computer is thinking…", "电脑正在思考…") }
        if winner != 0 {
            return winner == player ? ChessText.choose("You win!", "你赢了！") : ChessText.choose("Computer wins", "电脑获胜")
        }
        if finished { return ChessText.choose("Draw — the board is full", "和棋——棋盘已满") }
        if showingHints { return ChessText.choose("Hints are on · Preview only", "提示已开启 · 仅供参考") }
        return ChessText.choose("Your turn", "轮到你了")
    }

    static func coordinate(_ index: Int) -> String {
        "\(String(UnicodeScalar(65 + index / 15)!))\(index % 15 + 1)"
    }

    func select(_ index: Int) {
        guard !busy, !finished, cells.indices.contains(index), cells[index] == 0 else { return }
        hints = []
        if selected == index {
            place()
        } else {
            selected = index
        }
    }

    func place() {
        guard let index = selected, !busy, !finished else { return }
        guard engine.place(at: index) else {
            message = ChessText.choose("This move is forbidden for Black. Choose another intersection.", "此处为黑棋禁手，请选择其他交叉点。")
            return
        }
        selected = nil
        hints = []
        refresh()
        if engine.computerTurn { analyze() }
    }

    func place(at index: Int) {
        guard !busy, !finished, cells.indices.contains(index), cells[index] == 0 else { return }
        selected = index
        place()
    }

    func analyze() {
        guard !busy else { return }
        busy = true
        showingThinking = false
        thinkingDelay?.cancel()
        thinkingDelay = Task { [weak self] in
            do { try await Task.sleep(nanoseconds: 500_000_000) }
            catch { return }
            guard !Task.isCancelled, let self = self, self.busy else { return }
            self.showingThinking = true
        }
        engine.analyze { [weak self] in
            Task { @MainActor in
                guard let self = self else { return }
                self.thinkingDelay?.cancel()
                self.thinkingDelay = nil
                self.showingThinking = false
                self.busy = false
                self.refresh()
            }
        }
    }

    func toggleHints() {
        guard !busy, !finished else { return }
        if showingHints { hints = []; return }
        selected = nil
        busy = true
        loadingHints = true
        engine.hints { [weak self] values in
            Task { @MainActor in
                self?.hints = values.map(\.intValue)
                self?.busy = false
                self?.loadingHints = false
            }
        }
    }

    func undo() {
        guard canUndo else { return }
        engine.undo()
        selected = nil
        hints = []
        refresh()
    }

    func newGame(player: Int, forbidden: Bool) {
        guard !busy else { return }
        engine.start(withPlayer: player, forbidden: forbidden)
        selected = nil
        hints = []
        refresh()
        save()
    }

    func save() { engine.save() }

    private func refresh() {
        let nextCells = engine.cells.map(\.intValue)
        let nextMoves = engine.moves.map(\.intValue)
        if cells != nextCells { cells = nextCells }
        if moves != nextMoves { moves = nextMoves }
        if winner != engine.winner { winner = engine.winner }
        if player != engine.player { player = engine.player }
        if forbidden != engine.forbidden { forbidden = engine.forbidden }
    }
}

/// Temporary input locking must not dim controls on every computer reply.
/// Each label supplies its persistent unavailable appearance; disabled still
/// blocks touch and accessibility activation immediately.
private struct StableControlButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label.opacity(configuration.isPressed ? 0.8 : 1)
    }
}

struct ChessView: View {
    @ObservedObject var game: ChessGame
    @State private var showingHelp = false
    @State private var showingNewGame = false
    @Environment(\.dynamicTypeSize) private var typeSize
    private let accent = Color(red: 0.13, green: 0.38, blue: 0.32)

    var body: some View {
        GeometryReader { geometry in
            let sideBySide = geometry.size.width > geometry.size.height && !typeSize.isAccessibilitySize
            if UIDevice.current.userInterfaceIdiom == .phone && !typeSize.isAccessibilitySize {
                phoneLayout(landscape: sideBySide)
            } else {
                expandedLayout(sideBySide: sideBySide, height: geometry.size.height)
            }
        }
        .background(Color(.systemGroupedBackground).ignoresSafeArea())
        .tint(accent)
        .sheet(isPresented: $showingHelp) { help }
        .sheet(isPresented: $showingNewGame) { NewGameView(game: game) }
        .alert(ChessText.choose("Choose another move", "请选择其他落点"), isPresented: Binding(
            get: { game.message != nil }, set: { if !$0 { game.message = nil } }
        )) {
            Button(ChessText.choose("OK", "知道了"), role: .cancel) { game.message = nil }
        } message: { Text(game.message ?? "") }
    }

    private func expandedLayout(sideBySide: Bool, height: CGFloat) -> some View {
        ScrollView {
                VStack(alignment: .leading, spacing: 20) {
                    header
                    if sideBySide {
                        HStack(alignment: .top, spacing: 28) {
                            boardSection.frame(maxWidth: max(250, min(620, height - 130)))
                            controls.frame(maxWidth: 360)
                        }
                        .frame(maxWidth: .infinity)
                    } else {
                        boardSection
                        controls
                    }
                }
                .padding(20)
                .frame(maxWidth: sideBySide ? 1080 : 650)
                .frame(maxWidth: .infinity)
        }
    }

    /// Reserve space for the controls first; the board uses the remaining safe area.
    private func phoneLayout(landscape: Bool) -> some View {
        VStack(spacing: 8) {
            HStack {
                Text(ChessText.choose("Five in a Row", "五子棋"))
                    .font(.title2.bold()).accessibilityAddTraits(.isHeader)
                Spacer(minLength: 8)
                Button { showingHelp = true } label: {
                    Label(ChessText.choose("How to Play", "玩法说明"), systemImage: "questionmark.circle")
                        .font(.subheadline.weight(.semibold)).frame(minHeight: 44)
                }.accessibilityIdentifier("how-to-play")
            }
            if landscape {
                HStack(spacing: 20) {
                    phoneBoard
                    phoneControls.frame(maxWidth: 340)
                }
            } else {
                phoneBoard
                phoneControls
            }
        }
        .padding(.horizontal, 12).padding(.vertical, 8)
    }

    private var phoneBoard: some View {
        VStack(spacing: 6) {
            HStack(spacing: 6) {
                progressIndicator
                Text(game.status).font(.subheadline.weight(.semibold))
                    .lineLimit(1).minimumScaleFactor(0.8).accessibilityIdentifier("game-status")
                Spacer(minLength: 4)
                Text(ChessText.choose("\(game.moves.count) moves", "已落 \(game.moves.count) 子"))
                    .font(.caption.monospacedDigit()).foregroundColor(.secondary)
                    .accessibilityIdentifier("move-count")
            }
            GeometryReader { area in
                let side = max(1, min(area.size.width, area.size.height))
                ChessBoard(game: game)
                    .frame(width: side, height: side)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
            Text(game.showingHints
                 ? ChessText.choose("Hint numbers preview a possible continuation.", "提示数字为参考走法，不会改变棋局。")
                 : ChessText.choose("Tap to select. Tap the same point again to place.", "点击预选，再点同一位置落子。"))
                .font(.caption).foregroundColor(.secondary).lineLimit(2)
                .frame(maxWidth: .infinity, minHeight: 28)
        }
    }

    private var phoneControls: some View {
        VStack(spacing: 6) {
            HStack(spacing: 6) {
                phoneAction(ChessText.choose("Undo Turn", "撤销本回合"), id: "undo-turn", disabled: !game.hasUndoableMoves, perform: game.undo)
                phoneAction(game.showingHints ? ChessText.choose("Hide Hints", "关闭提示") : ChessText.choose("Show Hints", "显示提示"),
                            id: "show-hints", disabled: game.finished, perform: game.toggleHints)
            }
            newGameButton
            difficultyMenu
            HStack(spacing: 6) {
                Text(game.player == 1 ? ChessText.choose("You: Black", "你执黑棋") : ChessText.choose("You: White", "你执白棋"))
                Text("·")
                Text(game.forbidden ? ChessText.choose("Forbidden moves: On", "禁手：开") : ChessText.choose("Forbidden moves: Off", "禁手：关"))
                Spacer(minLength: 0)
            }
            .font(.caption).foregroundColor(.secondary)
        }
        .padding(10)
        .background(Color(.secondarySystemGroupedBackground), in: RoundedRectangle(cornerRadius: 16))
        .fixedSize(horizontal: false, vertical: true)
    }

    private var progressIndicator: some View {
        ZStack {
            if game.showingProgress { ProgressView().controlSize(.small) }
        }
        .frame(width: 16, height: 16)
        .accessibilityHidden(true)
    }

    private func phoneAction(_ title: String, id: String, disabled: Bool, perform: @escaping () -> Void) -> some View {
        Button(action: perform) {
            Text(title).font(.subheadline.weight(.semibold))
                .frame(maxWidth: .infinity, minHeight: 44).contentShape(Rectangle())
        }
        .buttonStyle(StableControlButtonStyle()).foregroundColor(disabled ? .secondary : accent)
        .background(accent.opacity(disabled ? 0.04 : 0.09), in: RoundedRectangle(cornerRadius: 10))
        .disabled(disabled || game.busy).accessibilityIdentifier(id)
    }

    private var newGameButton: some View {
        Button { showingNewGame = true } label: {
            Label(ChessText.choose("New Game", "重新开始一局"), systemImage: "arrow.clockwise.circle.fill")
                .font(.headline).frame(maxWidth: .infinity, minHeight: 52)
                .contentShape(Rectangle())
        }
        .buttonStyle(StableControlButtonStyle()).foregroundColor(.white)
        .background(accent, in: RoundedRectangle(cornerRadius: 12))
        .disabled(game.busy).accessibilityIdentifier("new-game")
    }

    private var difficultyMenu: some View {
        Menu {
            Picker(ChessText.choose("Difficulty", "电脑难度"), selection: Binding(
                get: { game.difficulty },
                // Also protect an already-open menu when a computer turn starts.
                set: { if !game.busy { game.difficulty = $0 } }
            )) {
                ForEach([2, 1, 0, 3, 4], id: \.self) { value in
                    Text(difficultyName(value)).tag(value)
                }
            }
        } label: {
            HStack {
                Text(ChessText.choose("Difficulty", "电脑难度"))
                    .font(.subheadline.weight(.semibold)).foregroundColor(.primary)
                Spacer(minLength: 4)
                Text(difficultyName(game.difficulty)).font(.subheadline)
                Image(systemName: "chevron.up.chevron.down").font(.caption)
            }
            .foregroundColor(accent)
            .frame(minHeight: 44).contentShape(Rectangle())
        }
        .buttonStyle(StableControlButtonStyle())
        .disabled(game.busy).accessibilityIdentifier("difficulty")
    }

    private var header: some View {
        HStack(alignment: .top) {
            VStack(alignment: .leading, spacing: 4) {
                Text(ChessText.choose("Five in a Row", "五子棋"))
                    .font(.largeTitle.bold()).accessibilityAddTraits(.isHeader)
                Text(ChessText.choose("A quiet moment. A clever move.", "静心思考，落子有方。"))
                    .font(.subheadline).foregroundColor(.secondary)
            }
            Spacer(minLength: 8)
            Button { showingHelp = true } label: {
                Label(ChessText.choose("How to Play", "玩法说明"), systemImage: "questionmark.circle")
                    .font(.subheadline.weight(.semibold))
                    .padding(.vertical, 10)
            }
            .accessibilityIdentifier("how-to-play")
        }
    }

    private var boardSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack(spacing: 10) {
                Circle().fill(game.player == 1 ? Color.black : Color.white)
                    .overlay(Circle().stroke(Color.gray, lineWidth: 1)).frame(width: 18, height: 18)
                    .accessibilityHidden(true)
                Text(game.player == 1 ? ChessText.choose("You · Black", "你 · 黑棋") : ChessText.choose("You · White", "你 · 白棋"))
                    .font(.subheadline.weight(.semibold))
                Spacer()
                Text(ChessText.choose("\(game.moves.count) moves", "已落 \(game.moves.count) 子"))
                    .font(.subheadline.monospacedDigit()).foregroundColor(.secondary)
                    .accessibilityIdentifier("move-count")
            }
            ChessBoard(game: game)
                .aspectRatio(1, contentMode: .fit)
            HStack(alignment: .top, spacing: 8) {
                Image(systemName: game.showingHints ? "eye" : "hand.tap")
                Text(game.showingHints
                     ? ChessText.choose("Numbers show a possible continuation, not guaranteed moves. Hide hints or select an empty intersection to continue.", "数字表示参考走法，并非必胜步骤。关闭提示或选择空白交叉点即可继续。")
                     : ChessText.choose("Tap an intersection to select it, then tap it again to place a stone.", "点击交叉点预选，再点同一位置即可落子。"))
            }
            .font(.footnote).foregroundColor(.secondary)
        }
    }

    private var controls: some View {
        VStack(alignment: .leading, spacing: 16) {
            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    progressIndicator
                    Text(game.status).font(.headline).accessibilityIdentifier("game-status")
                }
                Text(ChessText.choose("Controls will be available when analysis finishes. Higher levels may take longer.", "分析完成后即可继续操作。较高难度可能需要更多时间。"))
                    .font(.footnote).foregroundColor(.secondary)
                    .opacity(game.showingProgress ? 1 : 0)
                    .accessibilityHidden(!game.showingProgress)
                HStack(spacing: 12) {
                    action(ChessText.choose("Undo Turn", "撤销本回合"), icon: "arrow.uturn.backward", id: "undo-turn", disabled: !game.hasUndoableMoves, perform: game.undo)
                    action(game.showingHints ? ChessText.choose("Hide Hints", "关闭提示") : ChessText.choose("Show Hints", "显示提示"),
                           icon: game.showingHints ? "eye.slash" : "eye", id: "show-hints", disabled: game.finished, perform: game.toggleHints)
                }
                Text(ChessText.choose("Undo removes your last move and the computer’s reply.", "撤销会移除你的上一步和电脑的应手。"))
                    .font(.caption).foregroundColor(.secondary)
            }
            .padding(16).background(Color(.secondarySystemGroupedBackground), in: RoundedRectangle(cornerRadius: 18))

            VStack(alignment: .leading, spacing: 10) {
                difficultyMenu
                Text(ChessText.choose("Applies to the computer’s next move. Level 1 is easiest; level 5 thinks more deeply.", "从电脑的下一步开始生效。1 级最简单，5 级思考更深入。"))
                    .font(.footnote).foregroundColor(.secondary)
                Divider()
                Label(game.forbidden ? ChessText.choose("Forbidden moves: On", "禁手规则：开启") : ChessText.choose("Forbidden moves: Off", "禁手规则：关闭"), systemImage: "shield")
                    .font(.subheadline.weight(.medium))
                Text(ChessText.choose("Change your color and rules when starting a new game.", "开始新棋局时可更改执棋颜色和规则。"))
                    .font(.footnote).foregroundColor(.secondary)
                newGameButton
            }
            .padding(16).background(Color(.secondarySystemGroupedBackground), in: RoundedRectangle(cornerRadius: 18))
            Label(ChessText.choose("Played offline · Progress saved automatically", "离线对弈 · 自动保存进度"), systemImage: "checkmark.shield")
                .font(.caption).foregroundColor(.secondary).frame(maxWidth: .infinity)
        }
    }

    private func action(_ title: String, icon: String, id: String, disabled: Bool, perform: @escaping () -> Void) -> some View {
        Button(action: perform) {
            Label(title, systemImage: icon).font(.subheadline.weight(.semibold))
                .frame(maxWidth: .infinity, minHeight: 44).contentShape(Rectangle())
        }
        .buttonStyle(StableControlButtonStyle()).foregroundColor(disabled ? .secondary : accent)
        .background(accent.opacity(disabled ? 0.04 : 0.09), in: RoundedRectangle(cornerRadius: 10))
        .disabled(disabled || game.busy).accessibilityIdentifier(id)
    }

    private func difficultyName(_ value: Int) -> String {
        switch value {
        case 2: return ChessText.choose("1 · Beginner", "1 级 · 入门")
        case 1: return ChessText.choose("2 · Easy", "2 级 · 简单")
        case 3: return ChessText.choose("4 · Hard", "4 级 · 困难")
        case 4: return ChessText.choose("5 · Expert", "5 级 · 专家")
        default: return ChessText.choose("3 · Normal", "3 级 · 普通")
        }
    }

    private var help: some View {
        NavigationView {
            List {
                helpSection("Connect five", "五子连珠", "Make an unbroken line of five stones horizontally, vertically, or diagonally. Black moves first.", "将五颗棋子横向、纵向或斜向连成一线即可获胜。黑棋先行。")
                helpSection("Place a stone", "如何落子", "Tap an empty intersection to select it. The green ring is your preview. Tap the same point again to place your stone, or double-tap an empty point to select and place it. Selecting a different point moves the preview.", "点击空白交叉点进行预选，绿色圆环表示预选位置。再点同一位置即可落子，也可以直接双击空白位置落子。点击其他位置只会移动预选。")
                helpSection("Hints and undo", "提示与撤销", "Show Hints previews a possible numbered continuation without changing the game. Hide Hints returns to play. Undo Turn takes back your last move and any computer reply.", "“显示提示”用数字预览参考走法，不会改变棋局。“关闭提示”可返回对弈。“撤销本回合”会撤回你的上一步和电脑的应手。")
                helpSection("Difficulty and rules", "难度与规则", "All five difficulty levels are available in the Difficulty menu. Forbidden moves restrict Black’s double-three, double-four, and overline moves. Choose color and rules in New Game.", "“电脑难度”菜单提供全部五个等级。开启禁手后，黑棋受三三、四四和长连规则限制。在“新棋局”中选择颜色和规则。")
                helpSection("Your saved game", "保存棋局", "Your game is saved on this device automatically, including after each move. Starting a new game replaces the current board. No account or internet connection is needed.", "棋局会在每次落子后自动保存在本机。开始新棋局会替换当前棋局。无需账号或网络连接。")
            }
            .navigationTitle(ChessText.choose("How to Play", "玩法说明"))
            .toolbar { ToolbarItem(placement: .confirmationAction) {
                Button(ChessText.choose("Done", "完成")) { showingHelp = false }
            } }
        }.navigationViewStyle(.stack)
    }

    private func helpSection(_ title: String, _ chineseTitle: String, _ text: String, _ chineseText: String) -> some View {
        Section(header: Text(ChessText.choose(title, chineseTitle))) { Text(ChessText.choose(text, chineseText)) }
    }
}
