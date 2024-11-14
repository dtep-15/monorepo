use leptos::{prelude::*, task::spawn_local};

use crate::{api::{self, networks::NetworkConnection}, set_app_state, AppState};

#[component]
pub fn connecting(connection_request: NetworkConnection) -> impl IntoView {
	log::info!("Connecting to `{}` WiFi, password: `{:?}`", connection_request.name, connection_request.password);

	let network_name = connection_request.name.clone();

	let switch_app_state = set_app_state();

	spawn_local(async move {
		// TODO: handle failed connections
		api::retry(move || api::networks::connect(connection_request.clone())).await;

		switch_app_state(AppState::ConfigurationComplete);
	});

	view! {
		<dt-centered-col>
			<h1>
				Connecting to {network_name}
			</h1>
			<dt-loading-bar class="network-connecting-bar"></dt-loading-bar>
		</dt-centered-col>
	}
}
