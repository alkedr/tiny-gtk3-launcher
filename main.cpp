#include <gtk/gtk.h>
#include <vector>
#include <algorithm>
#include <fstream>


struct App {
	std::string name;
	std::string command;
	long launchesCount;

	bool operator<(const App & other) const {
		return launchesCount < other.launchesCount;
	}
};

static std::vector<App> loadApps() {
	std::vector<App> result;
	std::ifstream in("/home/alkedr/.config/tiny-gtk3-launcher");
	std::string line;
	if (!std::getline(in, line)) return result;
	int appsCount = std::stoi(line);
	for (auto i = 0; i < appsCount; i++) {
		std::string name, command, launchesCount;
		if (!std::getline(in, name)) return result;
		if (!std::getline(in, command)) return result;
		if (!std::getline(in, launchesCount)) return result;
		result.push_back({ name, command, stoi(launchesCount) });
	}
	return result;
}


static auto apps = loadApps();


static void saveApps() {
	std::ofstream out("/home/alkedr/.config/tiny-gtk3-launcher");
	out << apps.size() << std::endl;
	for (auto app : apps) {
		out << app.name << std::endl << app.command << std::endl << app.launchesCount << std::endl;
	}
}



static std::string makeMatchesBold(std::string appName, std::string query) {
	std::string result;
	for (
		auto itAppName = appName.cbegin(), itQuery = query.cbegin();
		(itAppName != appName.cend());
		itAppName++
	) {
		if ((itQuery != query.cend()) && (*itAppName == *itQuery)) {
			result += "<b>";
			result += std::string(1, *itAppName);
			result += "</b>";
			itQuery++;
		} else {
			result += std::string(1, *itAppName);
		}
	}
	return result;
}

static bool isMatch(std::string appName, std::string query, std::string appNameWithBoldMatches) {
	return query.empty() || (appNameWithBoldMatches.size() == (appName.size() + 7*query.size()));
}

static bool isMatch(std::string appName, std::string query) {
	return isMatch(appName, query, makeMatchesBold(appName, query));
}

static std::vector<App> searchApps(std::string query) {
	std::vector<App> result;
	for (auto app : apps) {
		if (isMatch(app.name, query)) {
			result.push_back(app);
		}
	}
	return result;
}


static void onSearchEntryChanged(GtkEditable * editable, gpointer userData) {
	auto store = GTK_LIST_STORE(userData);
	auto query = std::string(gtk_editable_get_chars(editable, 0, -1));
	auto matchingApps = searchApps(query);
	gtk_list_store_clear(store);
	for (size_t i = 0; i < std::min(matchingApps.size(), 10lu); i++) {
		char number[4];
		sprintf(number, "%lu", (i + 1) % 10);
		gtk_list_store_insert_with_values(store, NULL, -1,
			0, number,
			1, makeMatchesBold(matchingApps[i].name, query).data(),
			2, std::to_string(matchingApps[i].launchesCount).data(),
			-1
		);
	}
}


static void onSearchEntryActivated(GtkEntry * entry, gpointer userData) {
	auto query = std::string(gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1));

	for (auto & app : apps) {
		if (isMatch(app.name, query)) {
			app.launchesCount++;
			saveApps();
			system(app.command.data());
			gtk_main_quit();
		}
	}
}

int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);

	loadApps();

	auto store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	auto window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		auto boxAlignment = gtk_alignment_new(0.5, 0.5, 0.3f, 0.66f);   // TODO: golden ratio
			auto box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
				auto searchEntryAlignment = gtk_alignment_new(0.5, 0.5, 1, 1);
					auto searchEntry = gtk_search_entry_new();
				auto searchResults = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	gtk_container_add(GTK_CONTAINER(window), boxAlignment);
		gtk_container_add(GTK_CONTAINER(boxAlignment), box);
			gtk_container_add(GTK_CONTAINER(box), searchEntryAlignment);
				gtk_container_add(GTK_CONTAINER(searchEntryAlignment), searchEntry);
			gtk_container_add(GTK_CONTAINER(box), searchResults);

	gtk_widget_override_font(window, pango_font_description_from_string("Arial 24"));

	gtk_widget_set_can_focus(searchResults, FALSE);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(searchResults)), GTK_SELECTION_NONE);

	gtk_tree_view_append_column(
		GTK_TREE_VIEW(searchResults),
		gtk_tree_view_column_new_with_attributes("#", gtk_cell_renderer_text_new(), "text", 0, NULL)
	);

	gtk_tree_view_append_column(
		GTK_TREE_VIEW(searchResults),
		gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "markup", 1, NULL)
	);

	gtk_tree_view_append_column(
		GTK_TREE_VIEW(searchResults),
		gtk_tree_view_column_new_with_attributes("", gtk_cell_renderer_text_new(), "text", 2, NULL)
	);

	onSearchEntryChanged(GTK_EDITABLE(searchEntry), store);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	g_signal_connect(searchEntry, "changed", G_CALLBACK(onSearchEntryChanged), store);
	g_signal_connect(searchEntry, "activate", G_CALLBACK(onSearchEntryActivated), NULL);


	gtk_widget_show_all(window);

	gtk_main();

	saveApps();

	return 0;
}
