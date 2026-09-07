import SwiftUI

/// Occupied cells and a thinking computer disable input, not the stone's appearance.
private struct BoardCellButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
    }
}

struct ChessBoard: View {
    @ObservedObject var game: ChessGame

    var body: some View {
        GeometryReader { geometry in
            // Half a cell plus a small safety margin keeps edge stones and
            // their touch targets inside the board, without a coordinate gutter.
            let cell = geometry.size.width / 15.2
            let inset = cell * 0.6
            let end = inset + cell * 14
            ZStack {
                RoundedRectangle(cornerRadius: 16)
                    .fill(LinearGradient(colors: [Color(red: 0.92, green: 0.77, blue: 0.54),
                                                  Color(red: 0.78, green: 0.57, blue: 0.32)],
                                         startPoint: .topLeading, endPoint: .bottomTrailing))
                Path { path in
                    for index in 0..<15 {
                        let offset = inset + CGFloat(index) * cell
                        path.move(to: CGPoint(x: inset, y: offset))
                        path.addLine(to: CGPoint(x: end, y: offset))
                        path.move(to: CGPoint(x: offset, y: inset))
                        path.addLine(to: CGPoint(x: offset, y: end))
                    }
                }.stroke(Color.black.opacity(0.45), lineWidth: 0.7)
                ForEach([3, 7, 11], id: \.self) { x in
                    ForEach([3, 7, 11], id: \.self) { y in
                        Circle().fill(Color.black.opacity(0.6)).frame(width: 4, height: 4)
                            .position(x: inset + CGFloat(x) * cell, y: inset + CGFloat(y) * cell)
                    }
                }
                ForEach(0..<225, id: \.self) { index in
                    boardCell(index, size: cell)
                        .position(x: inset + CGFloat(index / 15) * cell, y: inset + CGFloat(index % 15) * cell)
                }
            }
            .shadow(color: .black.opacity(0.12), radius: 8, y: 4)
        }
        .accessibilityElement(children: .contain)
        .accessibilityIdentifier("chess-board")
    }

    private func boardCell(_ index: Int, size: CGFloat) -> some View {
        let value = game.cells[index]
        let hint = game.hints.count == 225 ? game.hints[index] : 0
        return Button { game.select(index) } label: {
            ZStack {
                Color.clear
                if value != 0 {
                    Circle()
                        .fill(LinearGradient(colors: value == 1 ? [Color(white: 0.28), .black] : [.white, Color(white: 0.82)],
                                             startPoint: .topLeading, endPoint: .bottomTrailing))
                        .overlay(Circle().stroke(Color.black.opacity(0.25), lineWidth: 0.5))
                        .shadow(color: .black.opacity(0.3), radius: size * 0.06, y: size * 0.04)
                        .padding(size * 0.07)
                    if game.moves.last == index {
                        Circle().stroke(value == 1 ? Color.white : Color.black, lineWidth: 1.5)
                            .frame(width: size * 0.3, height: size * 0.3)
                    }
                } else if hint != 0 {
                    Circle().fill(hint > 0 ? Color.black.opacity(0.75) : Color.white.opacity(0.9))
                        .padding(size * 0.08)
                    Text("\(abs(hint))").font(.system(size: size * 0.5, weight: .bold))
                        .foregroundColor(hint > 0 ? .white : .black)
                }
                if game.selected == index {
                    Circle().stroke(Color(red: 0.05, green: 0.42, blue: 0.3), lineWidth: 3)
                        .padding(1)
                    Circle().fill(Color(red: 0.05, green: 0.42, blue: 0.3)).frame(width: 5, height: 5)
                }
            }
            .frame(width: size, height: size).contentShape(Rectangle())
        }
        .buttonStyle(BoardCellButtonStyle())
        // A scroll view can coalesce rapid button activations. Recognize a real
        // double tap explicitly as well; the model rejects duplicate placement.
        .simultaneousGesture(TapGesture(count: 2).onEnded { game.place(at: index) })
        .disabled(game.busy || game.finished || value != 0)
        .accessibilityLabel("\(ChessGame.coordinate(index)), \(value == 0 ? ChessText.choose("empty", "空位") : value == 1 ? ChessText.choose("black stone", "黑棋") : ChessText.choose("white stone", "白棋"))")
        .accessibilityValue(game.selected == index ? ChessText.choose("Selected", "已选择") : "")
        .accessibilityHint(ChessText.choose("Activate to select; activate the selected point again to place a stone.", "激活以预选，再次激活同一位置即可落子。"))
        .accessibilityIdentifier("cell-\(index)")
    }
}

struct NewGameView: View {
    @ObservedObject var game: ChessGame
    @Environment(\.dismiss) private var dismiss
    @State private var player: Int
    @State private var forbidden: Bool

    init(game: ChessGame) {
        self.game = game
        _player = State(initialValue: game.player)
        _forbidden = State(initialValue: game.forbidden)
    }

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text(ChessText.choose("Your stones", "执棋颜色"))) {
                    Picker(ChessText.choose("Play as", "选择颜色"), selection: $player) {
                        Text(ChessText.choose("Black · Play first", "黑棋 · 先手")).tag(1)
                        Text(ChessText.choose("White · Play second", "白棋 · 后手")).tag(-1)
                    }
                    .pickerStyle(.inline).accessibilityIdentifier("player-color")
                }
                Section(header: Text(ChessText.choose("Game rules", "对局规则"))) {
                    Toggle(ChessText.choose("Forbidden Moves", "禁手规则"), isOn: $forbidden)
                        .accessibilityIdentifier("forbidden-moves")
                    Text(ChessText.choose("When enabled, Black cannot play double-three, double-four, or overline moves. White is unrestricted.", "开启后，黑棋不能落三三、四四或长连禁手。白棋不受禁手限制。"))
                        .font(.footnote).foregroundColor(.secondary)
                }
                Section {
                    Text(ChessText.choose("Starting a new game replaces your current board and saved progress.", "开始新棋局会替换当前棋局及其保存进度。"))
                        .foregroundColor(.secondary)
                    Button {
                        game.newGame(player: player, forbidden: forbidden)
                        dismiss()
                    } label: {
                        // A Form gives Label's icon its own leading column.
                        // Use text so the title itself stays centered in the button.
                        Text(ChessText.choose("Start New Game", "开始新棋局"))
                            .font(.headline)
                            .multilineTextAlignment(.center)
                            .frame(maxWidth: .infinity, minHeight: 52, alignment: .center)
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(game.busy).accessibilityIdentifier("start-new-game")
                }
            }
            .navigationTitle(ChessText.choose("New Game", "新棋局"))
            .toolbar { ToolbarItem(placement: .cancellationAction) {
                Button(ChessText.choose("Cancel", "取消")) { dismiss() }
            } }
        }.navigationViewStyle(.stack)
    }
}
