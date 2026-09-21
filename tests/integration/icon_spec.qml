import QtQuick
import Qcm.Material as MD

Item {
    id: root
    width: 800
    height: 600
    MD.Action { id: shared; icon.name: "home" }
    MD.Button { id: button; action: shared; mdState.size: MD.Enum.XS }
    MD.IconButton { id: iconButton; action: shared; mdState.size: MD.Enum.XL; checked: true }
    MD.StandardIconButton { id: standard; action: shared; iconFill: true }
    MD.BusyIconButton { action: shared }
    MD.SmallIconButton { action: shared }
    MD.TabButton { action: shared }
    MD.BarItem { action: shared }
    MD.RailItem { action: shared }
    MD.DrawerItem { action: shared }
    MD.AssistChip { action: shared }
    MD.SuggestionChip { action: shared }
    MD.InputChip { id: inputChip; action: shared }
    MD.FilterChip { action: shared }
    MD.EmbedChip { action: shared }
    MD.MenuItem { action: shared }
    MD.Switch { action: shared }
    MD.SegmentedButton { action: shared }
    MD.FAB { action: shared }
    MD.SplitButtonIndicator {}
    MD.ListItem { action: shared; index: 0; model: null }
    MD.SheetActionBar { actions: [shared] }
    MD.NavigationRail { fabAction: shared }

    MD.Button {
        id: imageButton
        icon.name: "home"
        icon.source: "../visual/assets/icon-colors.svg"
        icon.cache: false
        icon.color: "transparent"
    }
    MD.IconView { id: imageView; icon: imageButton.icon }
    readonly property bool imageReady: imageView.status === Image.Ready
    readonly property int imageStatus: imageView.status
    function failImage() { imageButton.icon.source = "../assets/missing.svg" }
    function showFont() { imageButton.icon.source = undefined }
    function fontShown() { return imageView.status === Image.Null && fontIcon(imageView) !== null }
    function clearImage() { imageButton.icon.name = "" }
    function emptyImage() { return imageView.implicitWidth === 0 && imageView.implicitHeight === 0 }

    function fontIcon(item) {
        if (item instanceof MD.Icon) return item;
        for (const child of item.children) {
            const found = fontIcon(child);
            if (found) return found;
        }
        return null;
    }

    function check() {
        if (button.icon.width !== button.mdState.iconSize) return "button token";
        if (iconButton.icon.width !== iconButton.mdState.iconSize) return "icon button token";
        if (!iconButton.icon.fill || !standard.icon.fill) return "fill defaults";
        shared.icon.fill = false;
        if (iconButton.icon.fill) return "explicit fill";
        if (standard.icon.fill) return "action must override control fill";
        shared.icon.fill = undefined;
        if (!iconButton.icon.fill || !standard.icon.fill) return "reset action fill";
        button.icon.width = 19;
        if (button.icon.width !== 19 || iconButton.icon.width !== iconButton.mdState.iconSize)
            return "control owns size";
        shared.icon.name = "edit";
        if (button.icon.name !== "edit") return "action name";
        shared.icon.name = "home";
        shared.icon.fill = false;
        shared.icon.color = "transparent";
        if (button.icon.color.a !== 0) return "transparent color";
        shared.icon.weight = Font.Bold;
        shared.icon.renderType = Text.NativeRendering;
        if (button.icon.weight !== Font.Bold) return "weight";
        if (button.icon.renderType !== Text.NativeRendering) return "render type";
        if (imageButton.icon.cache) return "image cache";
        const glyph = fontIcon(button.contentItem);
        if (!glyph || glyph.name !== "home" || glyph.weight !== Font.Bold
                || glyph.renderType !== Text.NativeRendering || glyph.fill !== false
                || glyph.color.a !== 0
                || glyph.size !== Math.min(button.icon.width, button.icon.height))
            return "font renderer";
        if (fontIcon(imageView)) return "image renderer";
        const chipGlyph = fontIcon(inputChip.contentItem);
        if (!chipGlyph || chipGlyph.size !== inputChip.icon.width)
            return "nested chip icon defaults";
        return "";
    }
}
