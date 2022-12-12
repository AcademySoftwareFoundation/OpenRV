import rvNuke

menubar = nuke.menu("Nuke")

menubar.addCommand("RV/Start RV", "rvNuke.startRv()", icon="rvNuke.png")
menubar.addCommand(
    "RV/Create Checkpoint", "rvNuke.createCheckpoint()", icon="rvNukeCheck.png"
)
menubar.addCommand("RV/View in RV", "rvNuke.viewReadsInRv(False)", icon="Viewer.png")
menubar.addCommand("RV/Render to RV", "rvNuke.Render()", icon="Write.png")
menubar.addCommand(
    "RV/Project Settings ...", "rvNuke.showSettingsPanel()", icon="Modify.png"
)
menubar.addCommand("RV/Preferences ...", "rvNuke.showPrefs()", icon="rvNukeGear.png")

toolbar = nuke.toolbar("RV")

toolbar.addCommand("Start RV", "rvNuke.startRv()", icon="rvNuke.png")
toolbar.addCommand(
    "Create Checkpoint", "rvNuke.createCheckpoint()", icon="rvNukeCheck.png"
)
toolbar.addCommand("View in RV", "rvNuke.viewReadsInRv(False)", icon="Viewer.png")
toolbar.addCommand("Render to RV", "rvNuke.Render()", icon="Write.png")
toolbar.addCommand(
    "Project Settings ...", "rvNuke.showSettingsPanel()", icon="Modify.png"
)
toolbar.addCommand("Preferences ...", "rvNuke.showPrefs()", icon="rvNukeGear.png")

if rvNuke.doDebug:
    menubar.addCommand("RV/Kill RV", "rvNuke.killRV()", icon="MergeMultiply.png")
    toolbar.addCommand("Kill RV", "rvNuke.killRV()", icon="MergeMultiply.png")

# propMenu = nuke.menu ('Animation')
# propMenu.addCommand ('rv test', 'rvNuke.testMenu()')
